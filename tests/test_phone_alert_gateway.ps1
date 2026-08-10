$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '..\tools\PhoneAlertGateway.psm1') -Force

function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

Assert-True ((Get-ServerChanEndpoint 'SCT123') -eq 'https://sctapi.ftqq.com/SCT123.send') 'Turbo endpoint mismatch'
Assert-True ((Get-ServerChanEndpoint 'sctp456tABC') -eq 'https://456.push.ft07.com/send/sctp456tABC.send') 'SC3 endpoint mismatch'

$invalidRejected = $false
try { $null = Get-ServerChanEndpoint 'invalid' } catch { $invalidRejected = $true }
Assert-True $invalidRejected 'Invalid SendKey was accepted'

$tracker = New-AlertTracker
$now = [datetime]'2026-08-10T00:00:00Z'
Assert-True ($null -eq (Add-AlertObservation $tracker STOP $now)) 'Initial state must be silent'
Assert-True ($null -eq (Add-AlertObservation $tracker STOP $now.AddSeconds(1))) 'Duplicate state must be silent'
$event = Add-AlertObservation $tracker RUN $now.AddSeconds(2)
Assert-True ($event.Sequence -eq 1 -and $tracker.Queue.Count -eq 1) 'Transition was not queued once'
$null = Add-AlertObservation $tracker OFFLINE $now.AddSeconds(3)
$null = Add-AlertObservation $tracker RUN $now.AddSeconds(4)
Assert-True ($tracker.Queue.Count -eq 3) 'Offline and recovery transitions were not retained'

$message = Get-AlertMessage $event 'test-device' ([pscustomobject]@{
    vibration_rms_g = 0.2; sensor_healthy = $true; sample_failed = 0; sample_missed = 0
})
Assert-True ($message.Title -eq '[RUNNING] test-device') 'Alert title mismatch'
Assert-True ($message.Description -match 'Sample failed/missed: 0/0') 'Alert details incomplete'

Write-Host 'Stage 11 phone alert gateway tests: PASS'
