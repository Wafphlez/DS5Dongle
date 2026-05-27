param(
    [int]$Port = 8080,
    [string]$BindAddress = "127.0.0.1"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$webDir = Join-Path $repoRoot "web"

if (-not (Test-Path $webDir)) {
    throw "Web folder not found: $webDir"
}

$pythonCmd = Get-Command "python" -ErrorAction SilentlyContinue
$usePyLauncher = $false
if (-not $pythonCmd) {
    $pythonCmd = Get-Command "py" -ErrorAction SilentlyContinue
    $usePyLauncher = $true
}

if (-not $pythonCmd) {
    throw "Python not found in PATH. Install Python 3 and rerun."
}

Set-Location $webDir

Write-Host "DS5 Bridge Config: http://$BindAddress`:$Port"
Write-Host "Press Ctrl+C to stop."
Write-Host "Open in Chrome or Edge after the DualSense is connected to the Pico."

if ($usePyLauncher) {
    & py -3 -m http.server $Port --bind $BindAddress
} else {
    & python -m http.server $Port --bind $BindAddress
}
