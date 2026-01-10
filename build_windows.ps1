<#
  Simple Windows local build script for nekoray (PowerShell, ASCII only)

  Usage (run from repo root):
    pwsh ./build_windows.ps1
    or
    powershell -ExecutionPolicy Bypass -File .\build_windows.ps1

  Parameters:
    -QtRoot           Qt SDK root directory (default: .\qt_lib\qt650)
    -Config           CMake configuration (Debug / Release / RelWithDebInfo / MinSizeRel)
    -DownloadCore     Download prebuilt nekobox_core from GitHub Releases
    -DownloadResources Download geosite/geodb resources (default: true)

  Steps:
    1. Configure CMake with Qt6 and minimal external deps
    2. Build nekobox (MSVC)
    3. Copy build\<Config>\nekobox.exe to repo root as nekobox.exe
    4. Download public resources (geosite.dat, geosite.db, geoip.dat, geoip.db)
    5. Optionally download prebuilt nekobox_core if -DownloadCore is specified
#>

param(
    [string]$QtRoot = "",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [switch]$DownloadCore = $false,
    [switch]$DownloadResources = $true
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message"
}

function Write-ErrorMsg {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Repo root (directory of this script)
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $RepoRoot

Write-Info "Repo root: $RepoRoot"

if (-not $QtRoot -or $QtRoot.Trim() -eq "") {
    # Default: repo-local Qt 6.5
    $QtRoot = Join-Path $RepoRoot "qt_lib\qt650"
}

if (-not (Test-Path -LiteralPath $QtRoot)) {
    Write-ErrorMsg "QtRoot '$QtRoot' does not exist. Please ensure Qt SDK is extracted there or pass -QtRoot explicitly."
    exit 1
}

Write-Info "Using QtRoot: $QtRoot"

# Check CMake
$cmakeVersion = & cmake --version 2>$null
if ($LASTEXITCODE -ne 0 -or -not $cmakeVersion) {
    Write-ErrorMsg "cmake not found or not working. Please install CMake and ensure it is in PATH."
    exit 1
}

Write-Info ("CMake OK: " + ($cmakeVersion -split "`n")[0])

$BuildDir = Join-Path $RepoRoot "build"

# Build Protobuf if gRPC is enabled
if (-not $env:NKR_NO_GRPC) {
    Write-Info "Building Protobuf dependency for gRPC..."
    $ProtobufScript = Join-Path $RepoRoot "libs\build_protobuf_windows.ps1"
    if (Test-Path $ProtobufScript) {
        & powershell -ExecutionPolicy Bypass -File $ProtobufScript
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Protobuf build failed, but continuing..."
        }
    } else {
        Write-Warning "Protobuf build script not found: $ProtobufScript"
    }
}

Write-Info "Configuring CMake (Config=$Config, QT_VERSION_MAJOR=6, gRPC enabled)..."

& cmake -S $RepoRoot -B $BuildDir `
    -DQT_VERSION_MAJOR=6 `
    -DNKR_NO_YAML=ON `
    -DNKR_NO_ZXING=ON `
    -DNKR_NO_QHOTKEY=ON `
    -DCMAKE_PREFIX_PATH="$QtRoot"

if ($LASTEXITCODE -ne 0) {
    Write-ErrorMsg "CMake configure failed (exit code=$LASTEXITCODE)."
    exit $LASTEXITCODE
}

Write-Info "Building (Config=$Config)..."

& cmake --build $BuildDir --config $Config

if ($LASTEXITCODE -ne 0) {
    Write-ErrorMsg "Build failed (exit code=$LASTEXITCODE)."
    exit $LASTEXITCODE
}

$BuiltExe = Join-Path $BuildDir "$Config\nekobox.exe"

if (-not (Test-Path -LiteralPath $BuiltExe)) {
    Write-ErrorMsg "Built exe not found at expected path: $BuiltExe"
    exit 1
}

Write-Info "Build success: $BuiltExe"

# Copy to repo root
$TargetExe = Join-Path $RepoRoot "nekobox.exe"

Write-Info "Copying to repo root: $TargetExe"

Copy-Item -LiteralPath $BuiltExe -Destination $TargetExe -Force

Write-Info "Done. nekobox.exe is available in repo root."

# Download public resources (geosite, geodb, etc.)
if ($DownloadResources) {
    Write-Info "Downloading public resources..."
    $DownloadScript = Join-Path $RepoRoot "libs\download_resources.ps1"
    $ReleaseDir = Join-Path $BuildDir $Config

    if (Test-Path -LiteralPath $DownloadScript) {
        & powershell -ExecutionPolicy Bypass -File $DownloadScript -DestDir $ReleaseDir
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[WARN] Failed to download public resources (exit code=$LASTEXITCODE). Continuing..." -ForegroundColor Yellow
        } else {
            Write-Info "Public resources downloaded to $ReleaseDir"
        }
    } else {
        Write-Host "[WARN] download_resources.ps1 not found, skipping resource download" -ForegroundColor Yellow
    }
}

# Download prebuilt nekobox_core if requested
if ($DownloadCore) {
    Write-Info "Downloading prebuilt nekobox_core..."
    $DownloadCoreScript = Join-Path $RepoRoot "libs\download_core.ps1"
    $ReleaseDir = Join-Path $BuildDir $Config

    if (Test-Path -LiteralPath $DownloadCoreScript) {
        & powershell -ExecutionPolicy Bypass -File $DownloadCoreScript -DestDir $ReleaseDir
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[WARN] Failed to download nekobox_core (exit code=$LASTEXITCODE). Continuing..." -ForegroundColor Yellow
        } else {
            Write-Info "nekobox_core downloaded successfully"
        }
    } else {
        Write-Host "[WARN] download_core.ps1 not found, skipping core download" -ForegroundColor Yellow
    }
}
