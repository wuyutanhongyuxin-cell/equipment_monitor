[CmdletBinding()]
param(
    [string]$StatusUri = 'http://192.168.101.19/status',
    [int]$PollSeconds = 2,
    [int]$StateConfirmSeconds = 10,
    [int]$OfflineSeconds = 30,
    [int]$MinimumSendIntervalSeconds = 60,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'PhoneAlertGateway.psm1') -Force

$localSecrets = Join-Path $PSScriptRoot 'serverchan.secrets.ps1'
if ([string]::IsNullOrWhiteSpace($env:SERVERCHAN_SENDKEY) -and (Test-Path -LiteralPath $localSecrets)) {
    . $localSecrets
}
if (-not $DryRun -and [string]::IsNullOrWhiteSpace($env:SERVERCHAN_SENDKEY)) {
    throw 'Set SERVERCHAN_SENDKEY in the process environment before starting the gateway.'
}

$tracker = New-AlertTracker
$lastSuccessUtc = [datetime]::UtcNow
$lastStatus = $null
$confirmedState = $null
$candidateState = $null
$candidateSinceUtc = $null

Write-Host "Stage 11 gateway started: $StatusUri"
Write-Host "Delivery mode: $(if ($DryRun) { 'dry-run' } else { 'ServerChan' })"

while ($true) {
    $now = [datetime]::UtcNow
    try {
        $status = Invoke-RestMethod -Uri $StatusUri -TimeoutSec ([Math]::Max(1, $PollSeconds))
        $lastStatus = $status
        $lastSuccessUtc = $now
        $observedState = if (-not [bool]$status.sensor_healthy) { 'SENSOR_FAULT' } else { [string]$status.state }
        if ($observedState -notin @('UNKNOWN', 'STOP', 'RUN')) { $observedState = 'UNKNOWN' }
        if ($null -eq $confirmedState) {
            $confirmedState = $observedState
            $null = Add-AlertObservation -Tracker $tracker -State $observedState -ObservedUtc $now
        } elseif ($observedState -eq $confirmedState) {
            $candidateState = $null
            $candidateSinceUtc = $null
        } elseif ($candidateState -ne $observedState) {
            $candidateState = $observedState
            $candidateSinceUtc = $now
        } elseif (($now - $candidateSinceUtc).TotalSeconds -ge $StateConfirmSeconds) {
            $confirmedState = $observedState
            $candidateState = $null
            $candidateSinceUtc = $null
            $null = Add-AlertObservation -Tracker $tracker -State $observedState -ObservedUtc $now
        }
    } catch {
        if (($now - $lastSuccessUtc).TotalSeconds -ge $OfflineSeconds) {
            if ($confirmedState -ne 'OFFLINE') {
                $confirmedState = 'OFFLINE'
                $candidateState = $null
                $candidateSinceUtc = $null
                $null = Add-AlertObservation -Tracker $tracker -State 'OFFLINE' -ObservedUtc $now
            }
        }
    }

    if ($tracker.Queue.Count -gt 0) {
        $intervalReady = $null -eq $tracker.LastDeliveredUtc -or
            ($now - $tracker.LastDeliveredUtc).TotalSeconds -ge $MinimumSendIntervalSeconds
        if ($intervalReady) {
            $event = $tracker.Queue.Peek()
            $message = Get-AlertMessage -Event $event -Device 'equipment-monitor-01' -Status $lastStatus
            try {
                if ($DryRun) {
                    Write-Host "DRY RUN: $($message.Title)"
                } else {
                    $endpoint = Get-ServerChanEndpoint -SendKey $env:SERVERCHAN_SENDKEY
                    $response = Invoke-RestMethod -Method Post -Uri $endpoint -Body @{
                        title = $message.Title
                        desp = $message.Description
                    } -TimeoutSec 15
                    if ([int]$response.code -ne 0) { throw "ServerChan returned code $($response.code)" }
                }
                $null = $tracker.Queue.Dequeue()
                $tracker.LastDeliveredUtc = $now
            } catch {
                Write-Warning "Alert delivery failed; queued for retry: $($_.Exception.Message)"
            }
        }
    }

    Start-Sleep -Seconds $PollSeconds
}
