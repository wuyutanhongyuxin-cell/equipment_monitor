$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$analyzer = Join-Path $repoRoot 'tools\analyze_threshold_dataset.ps1'
$separatedFixture = Join-Path $PSScriptRoot 'fixtures\threshold-separated.log'
$overlapFixture = Join-Path $PSScriptRoot 'fixtures\threshold-overlap.log'

$separated = & $analyzer -Path $separatedFixture -MinimumWindows 2 | ConvertFrom-Json
if (-not $separated.full_ranges_separated) {
    throw 'Separated fixture was rejected'
}
if ([Math]::Abs($separated.midpoint_candidate_g - 0.103) -gt 0.000001) {
    throw "Unexpected midpoint: $($separated.midpoint_candidate_g)"
}

$overlap = & $analyzer -Path $overlapFixture -MinimumWindows 2 | ConvertFrom-Json
if ($overlap.full_ranges_separated -or $null -ne $overlap.midpoint_candidate_g) {
    throw 'Overlapping fixture produced a threshold'
}

$minimumRejected = $false
try {
    & $analyzer -Path $separatedFixture -MinimumWindows 3 | Out-Null
} catch {
    $minimumRejected = $_.Exception.Message -like 'STOP has 2 valid windows*'
}
if (-not $minimumRejected) {
    throw 'Insufficient fixture was not rejected'
}

Write-Output 'Stage 9 threshold analyzer tests: PASS'
