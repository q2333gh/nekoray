# Build Protobuf for Windows (MSVC)
# Based on libs/build_deps_all.sh

param(
    [string]$BuildDir = "deps",
    [string]$InstallPrefix = "built"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DepsDir = Join-Path (Join-Path $RepoRoot "libs") $BuildDir
$InstallDir = Join-Path $DepsDir $InstallPrefix

Write-Host "Building Protobuf for Windows..." -ForegroundColor Cyan
Write-Host "Install prefix: $InstallDir" -ForegroundColor Gray

# Create directories
New-Item -ItemType Directory -Force -Path $DepsDir | Out-Null
Push-Location $DepsDir

try {
    # Clone protobuf
    if (-not (Test-Path "protobuf")) {
        Write-Host "Cloning protobuf v21.4..." -ForegroundColor Yellow
        git clone --recurse-submodules -b v21.4 --depth 1 --shallow-submodules https://github.com/protocolbuffers/protobuf
    }

    # Build protobuf
    $ProtobufBuildDir = Join-Path "protobuf" "build"
    New-Item -ItemType Directory -Force -Path $ProtobufBuildDir | Out-Null
    Push-Location $ProtobufBuildDir

    try {
        Write-Host "Configuring protobuf..." -ForegroundColor Yellow
        & cmake .. `
            -DCMAKE_BUILD_TYPE=Release `
            -DBUILD_SHARED_LIBS=OFF `
            -Dprotobuf_MSVC_STATIC_RUNTIME=OFF `
            -Dprotobuf_BUILD_TESTS=OFF `
            -DCMAKE_INSTALL_PREFIX="$InstallDir"

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed"
        }

        Write-Host "Building protobuf..." -ForegroundColor Yellow
        & cmake --build . --config Release

        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }

        Write-Host "Installing protobuf..." -ForegroundColor Yellow
        & cmake --install . --config Release

        if ($LASTEXITCODE -ne 0) {
            throw "Install failed"
        }

        Write-Host "Protobuf built and installed successfully!" -ForegroundColor Green
        Write-Host "Install directory: $InstallDir" -ForegroundColor Gray
    }
    finally {
        Pop-Location
    }
}
finally {
    Pop-Location
}
