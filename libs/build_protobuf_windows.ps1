# Build Protobuf for Windows (MSVC)
# Based on libs/build_deps_all.sh

param(
    [string]$BuildDir = "deps",
    [string]$InstallPrefix = "built"
)

$ErrorActionPreference = "Stop"

function Test-ProtobufInstall {
    param([string]$InstallDir)
    $ProtocPaths = @(
        (Join-Path $InstallDir "bin\protoc.exe"),
        (Join-Path $InstallDir "tools\protobuf\protoc.exe")
    )
    $LibPaths = @(
        (Join-Path $InstallDir "lib\libprotobuf.lib"),
        (Join-Path $InstallDir "lib\protobuf.lib")
    )
    $CmakePaths = @(
        (Join-Path $InstallDir "cmake\protobuf-config.cmake"),
        (Join-Path $InstallDir "share\protobuf\protobuf-config.cmake")
    )
    $HasProtoc = $false
    foreach ($p in $ProtocPaths) { if (Test-Path -LiteralPath $p) { $HasProtoc = $true; break } }
    $HasLib = $false
    foreach ($p in $LibPaths) { if (Test-Path -LiteralPath $p) { $HasLib = $true; break } }
    $HasCmake = $false
    foreach ($p in $CmakePaths) { if (Test-Path -LiteralPath $p) { $HasCmake = $true; break } }
    return $HasProtoc -and $HasLib -and $HasCmake
}

function Import-VsDevCmdEnv {
    param([string]$VsDevCmd)
    $EnvOutput = & cmd /c "`"$VsDevCmd`" -no_logo && set"
    foreach ($line in $EnvOutput) {
        if ($line -match "^(.*?)=(.*)$") {
            Set-Item -Path ("env:" + $matches[1]) -Value $matches[2]
        }
    }
}

function Invoke-CMake {
    param([string[]]$CmakeArgs)
    & cmake @CmakeArgs
}

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DepsDir = Join-Path (Join-Path $RepoRoot "libs") $BuildDir
$InstallDir = Join-Path $DepsDir $InstallPrefix
$Ninja = Get-Command ninja -ErrorAction SilentlyContinue
$IsNinja = $null -ne $Ninja
$Generator = ""
if ($IsNinja) {
    $Generator = "Ninja Multi-Config"
    Write-Host "Using generator: $Generator" -ForegroundColor Gray
}
$script:UseVsDevCmd = $false
$script:VsDevCmd = $null
if ($IsNinja) {
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $VsWhere) {
        $VsInstall = & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($VsInstall) {
            $VsDevCmd = Join-Path $VsInstall "Common7\Tools\VsDevCmd.bat"
            if (Test-Path -LiteralPath $VsDevCmd) {
                $script:UseVsDevCmd = $true
                $script:VsDevCmd = $VsDevCmd
            }
        }
    }
    if (-not $script:UseVsDevCmd) {
        Write-Host "Ninja detected but Visual Studio dev environment not found; falling back to Visual Studio generator." -ForegroundColor Yellow
        $IsNinja = $false
        $Generator = ""
    }
}

$IsNinja = $IsNinja -and $script:UseVsDevCmd
if ($IsNinja) {
    Import-VsDevCmdEnv -VsDevCmd $script:VsDevCmd
}

Write-Host "Building Protobuf for Windows..." -ForegroundColor Cyan
Write-Host "Install prefix: $InstallDir" -ForegroundColor Gray
if (Test-ProtobufInstall -InstallDir $InstallDir) {
    Write-Host "Protobuf already present in install prefix, skipping build." -ForegroundColor Green
    return
}

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
    $ProtobufBuildDir = Join-Path "protobuf" $(if ($IsNinja) { "build-ninja" } else { "build" })
    New-Item -ItemType Directory -Force -Path $ProtobufBuildDir | Out-Null
    Push-Location $ProtobufBuildDir

    try {
        Write-Host "Configuring protobuf..." -ForegroundColor Yellow
        if ($Generator) {
            Invoke-CMake -CmakeArgs @("-G", $Generator, "..",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_SHARED_LIBS=OFF",
                "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF",
                "-Dprotobuf_BUILD_TESTS=OFF",
                "-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false",
                "-DCMAKE_INSTALL_PREFIX=$InstallDir")
        } else {
            Invoke-CMake -CmakeArgs @("..",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_SHARED_LIBS=OFF",
                "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF",
                "-Dprotobuf_BUILD_TESTS=OFF",
                "-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false",
                "-DCMAKE_INSTALL_PREFIX=$InstallDir")
        }

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed"
        }

        Write-Host "Building protobuf..." -ForegroundColor Yellow
        if ($IsNinja) {
            Invoke-CMake -CmakeArgs @("--build", ".", "--config", "Release")
        } else {
            Invoke-CMake -CmakeArgs @("--build", ".", "--config", "Release", "--", "/p:TrackFileAccess=false", "/p:EnableMinimalRebuild=false")
        }

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
