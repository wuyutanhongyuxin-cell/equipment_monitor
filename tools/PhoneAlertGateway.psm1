Set-StrictMode -Version Latest

function Get-ServerChanEndpoint {
    param([Parameter(Mandatory)][string]$SendKey)

    if ($SendKey -match '^sctp([0-9]+)t') {
        return "https://$($Matches[1]).push.ft07.com/send/$SendKey.send"
    }
    if ($SendKey -match '^SCT') {
        return "https://sctapi.ftqq.com/$SendKey.send"
    }
    throw 'Unsupported ServerChan SendKey prefix. Expected SCT or sctp.'
}

function New-AlertTracker {
    [pscustomobject]@{
        Initialized = $false
        Current = 'UNKNOWN'
        Sequence = [uint32]0
        Queue = [System.Collections.Generic.Queue[object]]::new()
        LastDeliveredUtc = $null
    }
}

function Add-AlertObservation {
    param(
        [Parameter(Mandatory)]$Tracker,
        [Parameter(Mandatory)][ValidateSet('UNKNOWN', 'STOP', 'RUN', 'SENSOR_FAULT', 'OFFLINE')][string]$State,
        [Parameter(Mandatory)][datetime]$ObservedUtc
    )

    if (-not $Tracker.Initialized) {
        $Tracker.Initialized = $true
        $Tracker.Current = $State
        return $null
    }
    if ($State -eq $Tracker.Current) { return $null }

    $Tracker.Sequence++
    $event = [pscustomobject]@{
        Sequence = $Tracker.Sequence
        From = $Tracker.Current
        To = $State
        ObservedUtc = $ObservedUtc
    }
    $Tracker.Current = $State
    $Tracker.Queue.Enqueue($event)
    return $event
}

function Get-AlertMessage {
    param(
        [Parameter(Mandatory)]$Event,
        [Parameter(Mandatory)][string]$Device,
        $Status
    )

    $label = switch ($Event.To) {
        'RUN' { 'RUNNING' }
        'STOP' { 'STOP ALERT' }
        'SENSOR_FAULT' { 'SENSOR FAULT' }
        'OFFLINE' { 'MONITOR OFFLINE' }
        default { 'STATE UNKNOWN' }
    }
    $title = "[$label] $Device"
    $lines = @(
        "Device: $Device"
        "Transition: $($Event.From) -> $($Event.To)"
        "Observed (UTC): $($Event.ObservedUtc.ToString('o'))"
        "Event sequence: $($Event.Sequence)"
    )
    if ($null -ne $Status) {
        $lines += "Vibration RMS: $($Status.vibration_rms_g) g"
        $lines += "Sensor healthy: $($Status.sensor_healthy)"
        $lines += "Sample failed/missed: $($Status.sample_failed)/$($Status.sample_missed)"
    }
    [pscustomobject]@{ Title = $title; Description = ($lines -join "`n`n") }
}

Export-ModuleMember -Function Get-ServerChanEndpoint, New-AlertTracker, Add-AlertObservation, Get-AlertMessage
