[CmdletBinding()]
param(
    [string]$StatusUri = 'http://192.168.101.19/status',
    [int]$PollSeconds = 2,
    [int]$StateConfirmSeconds = 10,
    [int]$OfflineSeconds = 30,
    [int]$MinimumSendIntervalSeconds = 60,
    [string]$StatusProxy = $env:HTTP_PROXY,
    [string]$ProviderProxy = '',
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
$lastPollWarningUtc = [datetime]::MinValue

Write-Host "Stage 11 gateway started: $StatusUri"
Write-Host "Delivery mode: $(if ($DryRun) { 'dry-run' } else { 'ServerChan' })"
Write-Host "Status proxy: $(if ([string]::IsNullOrWhiteSpace($StatusProxy)) { 'disabled' } else { 'enabled' })"

while ($true) {
    $now = [datetime]::UtcNow
    try {
        $statusRequest = @{
            Uri = $StatusUri
            TimeoutSec = [Math]::Max(1, $PollSeconds)
        }
        if (-not [string]::IsNullOrWhiteSpace($StatusProxy)) {
            $statusRequest.Proxy = $StatusProxy
        }
        $status = Invoke-RestMethod @statusRequest
        $lastStatus = $status
        $lastSuccessUtc = $now
        $observedState = if (-not [bool]$status.sensor_healthy) { 'SENSOR_FAULT' } else { [string]$status.state }
        if ($observedState -notin @('UNKNOWN', 'STOP', 'RUN')) { $observedState = 'UNKNOWN' }
        if ($null -eq $confirmedState) {
            $confirmedState = $observedState
            $null = Add-AlertObservation -Tracker $tracker -State $observedState -ObservedUtc $now
            Write-Host "Initial state: $confirmedState (silent baseline)"
        } elseif ($observedState -eq $confirmedState) {
            if ($null -ne $candidateState) {
                Write-Host "Candidate cancelled: $candidateState"
            }
            $candidateState = $null
            $candidateSinceUtc = $null
        } elseif ($candidateState -ne $observedState) {
            $candidateState = $observedState
            $candidateSinceUtc = $now
            Write-Host "Candidate state: $candidateState"
        } elseif (($now - $candidateSinceUtc).TotalSeconds -ge $StateConfirmSeconds) {
            $confirmedState = $observedState
            $candidateState = $null
            $candidateSinceUtc = $null
            $null = Add-AlertObservation -Tracker $tracker -State $observedState -ObservedUtc $now
            Write-Host "Confirmed state: $confirmedState"
        }
    } catch {
        if (($now - $lastPollWarningUtc).TotalSeconds -ge 30) {
            Write-Warning "Status poll failed: $($_.Exception.Message)"
            $lastPollWarningUtc = $now
        }
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
                    $providerRequest = @{
                        Method = 'Post'
                        Uri = $endpoint
                        Body = @{
                            title = $message.Title
                            desp = $message.Description
                        }
                        TimeoutSec = 15
                    }
                    if (-not [string]::IsNullOrWhiteSpace($ProviderProxy)) {
                        $providerRequest.Proxy = $ProviderProxy
                    }
                    $response = Invoke-RestMethod @providerRequest
                    if ([int]$response.code -ne 0) { throw "ServerChan returned code $($response.code)" }
                }
                $null = $tracker.Queue.Dequeue()
                $tracker.LastDeliveredUtc = $now
                Write-Host "Delivered: $($message.Title)"
            } catch {
                Write-Warning "Alert delivery failed; queued for retry: $($_.Exception.Message)"
            }
        }
    }

    Start-Sleep -Seconds $PollSeconds
}
