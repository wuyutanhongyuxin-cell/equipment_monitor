$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $PSScriptRoot 'test_local_indicator_policy.cpp'
$output = Join-Path $env:TEMP 'equipment_monitor_indicator_policy_test.exe'

try {
    & g++ -std=c++17 -Wall -Wextra -Werror -I $repoRoot $source -o $output
    if ($LASTEXITCODE -ne 0) { throw "g++ failed with exit code $LASTEXITCODE" }
    & $output
    if ($LASTEXITCODE -ne 0) { throw "indicator policy test failed with exit code $LASTEXITCODE" }
} finally {
    Remove-Item -LiteralPath $output -Force -ErrorAction SilentlyContinue
}
