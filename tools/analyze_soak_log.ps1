param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [ValidateRange(1, 10000000)]
    [int]$MinimumWindows = 86400
)

$resolvedPath = Resolve-Path -LiteralPath $Path -ErrorAction Stop
$states = @{ UNKNOWN = 0; STOP = 0; RUN = 0 }
$validWindows = 0
$invalidWindows = 0
$failedSamples = 0
$missedPeriods = 0
$minimumRms = [double]::PositiveInfinity
$maximumRms = 0.0

foreach ($line in Get-Content -LiteralPath $resolvedPath) {
    if ($line -notmatch '^state=(UNKNOWN|STOP|RUN),rms=([0-9]+(?:\.[0-9]+)?),.*valid=([0-9]+)/200,failed=([0-9]+),missed=([0-9]+)') {
        if ($line -like 'classifier: invalid window*') { $invalidWindows++ }
        continue
    }

    $state = $Matches[1]
    $rms = [double]::Parse($Matches[2], [Globalization.CultureInfo]::InvariantCulture)
    $valid = [int]$Matches[3]
    $failed = [int]$Matches[4]
    $missed = [int]$Matches[5]
    $states[$state]++
    $failedSamples += $failed
    $missedPeriods += $missed
    $minimumRms = [Math]::Min($minimumRms, $rms)
    $maximumRms = [Math]::Max($maximumRms, $rms)

    if ($valid -eq 200 -and $failed -eq 0 -and $missed -eq 0) {
        $validWindows++
    } else {
        $invalidWindows++
    }
}

if ($validWindows -lt $MinimumWindows) {
    throw "Only $validWindows valid windows; minimum is $MinimumWindows"
}

$passed = $invalidWindows -eq 0 -and $failedSamples -eq 0 -and $missedPeriods -eq 0
$result = [ordered]@{
    source = $resolvedPath.Path
    passed = $passed
    valid_windows = $validWindows
    invalid_windows = $invalidWindows
    failed_samples = $failedSamples
    missed_periods = $missedPeriods
    states = $states
    minimum_rms_g = $minimumRms
    maximum_rms_g = $maximumRms
}

$result | ConvertTo-Json -Depth 3
if (-not $passed) { exit 2 }
