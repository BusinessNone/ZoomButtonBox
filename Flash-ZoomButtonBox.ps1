<#
.SYNOPSIS
    Compiles ZoomButtonsNew.ino and uploads it to a Seeed XIAO SAMD21.

.DESCRIPTION
    Stages the sketch into a folder whose name matches the .ino (arduino-cli
    requirement), compiles for Seeeduino:samd:seeed_XIAO_m0, and uploads to the
    XIAO it finds (or the port you pass). Use -CompileOnly to skip the upload.

.PARAMETER Port
    Serial port of the XIAO (e.g. COM7). Auto-detected when omitted.

.PARAMETER CompileOnly
    Build without uploading.

.EXAMPLE
    .\Flash-ZoomButtonBox.ps1
.EXAMPLE
    .\Flash-ZoomButtonBox.ps1 -Port COM7
#>
[CmdletBinding()]
param(
    [string]$Port,
    [switch]$CompileOnly
)

$ErrorActionPreference = 'Stop'
$Fqbn   = 'Seeeduino:samd:seeed_XIAO_m0'
$Root   = $PSScriptRoot
$Stage  = Join-Path $Root 'build\ZoomButtonsNew'
$Output = Join-Path $Root 'build\out'

$cli = (Get-Command arduino-cli -ErrorAction SilentlyContinue).Source
if (-not $cli) { $cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe' }
if (-not (Test-Path $cli)) { throw 'arduino-cli not found. Install with: winget install ArduinoSA.CLI' }

# Self-contained toolchain: config, cores and libraries live in .arduino\ inside
# the repo, so the script works the same from any shell (no AppData dependency).
$ArduinoDir = Join-Path $Root '.arduino'
$Config     = Join-Path $ArduinoDir 'arduino-cli.yaml'
$SeeedUrl   = 'https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json'
if (-not (Test-Path $Config)) {
    New-Item -ItemType Directory -Force $ArduinoDir | Out-Null
    $d = $ArduinoDir -replace '\\', '/'
    @"
board_manager:
  additional_urls:
    - $SeeedUrl
directories:
  data: $d/data
  downloads: $d/staging
  user: $d/user
"@ | Set-Content -Path $Config -Encoding ascii
}
function Invoke-Cli { & $cli --config-file $Config @args }

$cores = Invoke-Cli core list --format json | ConvertFrom-Json
if (-not ($cores.platforms.id -contains 'Seeeduino:samd')) {
    Write-Host 'Installing Seeed SAMD core (first run only)...' -ForegroundColor Cyan
    Invoke-Cli core update-index
    Invoke-Cli core install Seeeduino:samd
    if ($LASTEXITCODE -ne 0) { throw 'Seeed SAMD core install failed.' }
}
$libs = Invoke-Cli lib list --format json | ConvertFrom-Json
if (-not ($libs.installed_libraries.library.name -contains 'Keyboard')) {
    Write-Host 'Installing Keyboard library (first run only)...' -ForegroundColor Cyan
    Invoke-Cli lib install Keyboard
    if ($LASTEXITCODE -ne 0) { throw 'Keyboard library install failed.' }
}

Write-Host 'Staging sketch...' -ForegroundColor Cyan
New-Item -ItemType Directory -Force $Stage | Out-Null
Copy-Item (Join-Path $Root 'ZoomButtonsNew.ino') $Stage -Force

Write-Host "Compiling for $Fqbn..." -ForegroundColor Cyan
Invoke-Cli compile --fqbn $Fqbn --warnings all --output-dir $Output $Stage
if ($LASTEXITCODE -ne 0) { throw 'Compile failed.' }
if ($CompileOnly) { Write-Host 'Compile OK.' -ForegroundColor Green; return }

if (-not $Port) {
    $boards = Invoke-Cli board list --format json | ConvertFrom-Json
    $match  = @($boards.detected_ports | Where-Object { $_.matching_boards.fqbn -contains $Fqbn })
    if ($match.Count -eq 0) {
        throw 'No XIAO found. Plug it in, or double-tap RST (bridge the RST pads twice quickly) to enter bootloader, then retry.'
    }
    $Port = $match[0].port.address
}

Write-Host "Uploading to $Port..." -ForegroundColor Cyan
Invoke-Cli upload --fqbn $Fqbn --port $Port --input-dir $Output $Stage
if ($LASTEXITCODE -ne 0) { throw 'Upload failed. Try double-tapping RST to force the bootloader, then rerun.' }
Write-Host 'Flashed. The box now enumerates as a USB keyboard.' -ForegroundColor Green
