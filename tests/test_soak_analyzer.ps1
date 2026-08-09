$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$analyzer = Join-Path $repoRoot 'tools\analyze_soak_log.ps1'
$healthyFixture = Join-Path $PSScriptRoot 'fixtures\soak-healthy.log'
$invalidFixture = Join-Path $PSScriptRoot 'fixtures\soak-invalid.log'

$healthy = & $analyzer -Path $healthyFixture -MinimumWindows 3 | ConvertFrom-Json
if (-not $healthy.passed -or $healthy.valid_windows -ne 3) {
    throw 'Healthy soak fixture failed'
}

$previousErrorAction = $ErrorActionPreference
$ErrorActionPreference = 'Continue'

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $analyzer `
    -Path $invalidFixture -MinimumWindows 1 2>$null | Out-Null
$invalidExitCode = $LASTEXITCODE

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $analyzer `
    -Path $healthyFixture -MinimumWindows 4 2>$null | Out-Null
$shortExitCode = $LASTEXITCODE

$ErrorActionPreference = $previousErrorAction

if ($invalidExitCode -ne 2) {
    throw "Invalid soak fixture exit code was $invalidExitCode, expected 2"
}

if ($shortExitCode -eq 0) {
    throw 'Short soak fixture was accepted'
}

Write-Output 'Stage 13 soak analyzer tests: PASS'
