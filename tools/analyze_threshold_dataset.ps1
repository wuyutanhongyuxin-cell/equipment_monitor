param(
    [Parameter(Mandatory = $true)]
    [string]$Path,

    [ValidateRange(2, 100000)]
    [int]$MinimumWindows = 60
)

$resolvedPath = Resolve-Path -LiteralPath $Path -ErrorAction Stop
$samples = @{ STOP = @(); RUN = @() }
$rejected = 0

foreach ($line in Get-Content -LiteralPath $resolvedPath) {
    if ($line -notmatch '^data,label=(STILL|STOP|VIBRATION|RUN),') {
        continue
    }

    $label = switch ($Matches[1]) {
        { $_ -in @('STILL', 'STOP') } { 'STOP'; break }
        default { 'RUN' }
    }

    if ($line -notmatch 'valid=200/200,failed=0,missed=0') {
        $rejected++
        continue
    }
    if ($line -notmatch '(?:rms_g|rms)=([0-9]+(?:\.[0-9]+)?)') {
        $rejected++
        continue
    }

    $samples[$label] += [double]::Parse(
        $Matches[1],
        [Globalization.CultureInfo]::InvariantCulture
    )
}

function Get-Percentile([double[]]$Values, [double]$Percentile) {
    $sorted = @($Values | Sort-Object)
    $index = [Math]::Ceiling(($Percentile / 100.0) * $sorted.Count) - 1
    $index = [Math]::Max(0, [Math]::Min($index, $sorted.Count - 1))
    return $sorted[$index]
}

foreach ($label in @('STOP', 'RUN')) {
    if ($samples[$label].Count -lt $MinimumWindows) {
        throw "$label has $($samples[$label].Count) valid windows; minimum is $MinimumWindows"
    }
}

$stopStats = $samples.STOP | Measure-Object -Minimum -Maximum -Average
$runStats = $samples.RUN | Measure-Object -Minimum -Maximum -Average
$separationGap = $runStats.Minimum - $stopStats.Maximum
$separated = $separationGap -gt 0

$result = [ordered]@{
    source = $resolvedPath.Path
    rejected_windows = $rejected
    stop = [ordered]@{
        count = $stopStats.Count
        minimum_rms_g = $stopStats.Minimum
        average_rms_g = $stopStats.Average
        percentile_95_rms_g = Get-Percentile $samples.STOP 95
        maximum_rms_g = $stopStats.Maximum
    }
    run = [ordered]@{
        count = $runStats.Count
        minimum_rms_g = $runStats.Minimum
        percentile_05_rms_g = Get-Percentile $samples.RUN 5
        average_rms_g = $runStats.Average
        maximum_rms_g = $runStats.Maximum
    }
    full_ranges_separated = $separated
    separation_gap_g = $separationGap
    midpoint_candidate_g = if ($separated) {
        ($stopStats.Maximum + $runStats.Minimum) / 2.0
    } else {
        $null
    }
}

$result | ConvertTo-Json -Depth 4
