# Build yaml-cpp and zxing-cpp for Windows (MSVC + Ninja)

param(
    [string]$BuildDir = "libs\\deps",
    [string]$InstallPrefix = "built"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Import-VsDevCmdEnv {
    param([string]$VsDevCmd)
    $EnvOutput = & cmd /c "`"$VsDevCmd`" -no_logo -arch=amd64 -host_arch=amd64 && set"
    foreach ($line in $EnvOutput) {
        if ($line -match "^(.*?)=(.*)$") {
            Set-Item -Path ("env:" + $matches[1]) -Value $matches[2]
        }
    }
}

function Ensure-VsDevCmd {
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $VsWhere)) {
        throw "vswhere.exe not found"
    }
    $VsInstall = & $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if (-not $VsInstall) {
        throw "Visual Studio installation not found"
    }
    $VsDevCmd = Join-Path $VsInstall "Common7\Tools\VsDevCmd.bat"
    if (-not (Test-Path -LiteralPath $VsDevCmd)) {
        throw "VsDevCmd.bat not found"
    }
    Import-VsDevCmdEnv -VsDevCmd $VsDevCmd
}

function Ensure-Repo {
    param([string]$Path, [string]$Url, [string]$Tag)
    if (-not (Test-Path -LiteralPath $Path)) {
        if ($Tag -eq "master" -or $Tag -eq "main") {
            git clone --depth 1 $Url $Path
        } else {
            git clone -b $Tag --depth 1 $Url $Path
        }
    }
}

$YamlConfigs = @(
    "lib\\cmake\\yaml-cpp\\yaml-cpp-config.cmake",
    "lib\\cmake\\yaml-cpp\\yaml-cppConfig.cmake",
    "cmake\\yaml-cpp\\yaml-cpp-config.cmake",
    "cmake\\yaml-cpp\\yaml-cppConfig.cmake",
    "share\\yaml-cpp\\yaml-cpp-config.cmake",
    "share\\yaml-cpp\\yaml-cppConfig.cmake"
)
$ZxingConfigs = @(
    "lib\\cmake\\ZXing\\ZXingConfig.cmake",
    "lib\\cmake\\ZXing\\ZXing-config.cmake",
    "cmake\\ZXing\\ZXingConfig.cmake",
    "cmake\\ZXing\\ZXing-config.cmake",
    "share\\zxing-cpp\\zxing-cpp-config.cmake",
    "share\\ZXing\\ZXingConfig.cmake"
)

function Test-CmakeConfig {
    param([string]$InstallDir, [string[]]$Candidates)
    foreach ($rel in $Candidates) {
        $p = Join-Path $InstallDir $rel
        if (Test-Path -LiteralPath $p) {
            return $true
        }
    }
    return $false
}

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DepsDir = Join-Path $RepoRoot $BuildDir
$InstallDir = Join-Path $DepsDir $InstallPrefix

New-Item -ItemType Directory -Force -Path $DepsDir | Out-Null
Ensure-VsDevCmd

Write-Host "Deps dir: $DepsDir"
Write-Host "Install prefix: $InstallDir"

if (-not (Test-CmakeConfig -InstallDir $InstallDir -Candidates $ZxingConfigs)) {
    Ensure-Repo -Path (Join-Path $DepsDir "zxing-cpp") -Url "https://github.com/nu-book/zxing-cpp" -Tag "v2.0.0"
}
if (-not (Test-CmakeConfig -InstallDir $InstallDir -Candidates $YamlConfigs)) {
    Ensure-Repo -Path (Join-Path $DepsDir "yaml-cpp") -Url "https://github.com/jbeder/yaml-cpp" -Tag "master"
}

if (-not (Test-CmakeConfig -InstallDir $InstallDir -Candidates $ZxingConfigs)) {
    # Build zxing-cpp
    $ZxingSrc = Join-Path $DepsDir "zxing-cpp"
    $ZxingBuild = Join-Path $ZxingSrc "build"
    New-Item -ItemType Directory -Force -Path $ZxingBuild | Out-Null
    Push-Location $ZxingBuild
    try {
        & cmake -G Ninja -S $ZxingSrc -B $ZxingBuild `
            -DBUILD_SHARED_LIBS=OFF `
            -DBUILD_EXAMPLES=OFF `
            -DBUILD_BLACKBOX_TESTS=OFF `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_INSTALL_PREFIX="$InstallDir"
        & cmake --build $ZxingBuild --config Release
        & cmake --install $ZxingBuild --config Release
    } finally {
        Pop-Location
    }
}

if (-not (Test-CmakeConfig -InstallDir $InstallDir -Candidates $YamlConfigs)) {
    # Build yaml-cpp
    $YamlSrc = Join-Path $DepsDir "yaml-cpp"
    $YamlBuild = Join-Path $YamlSrc "build"
    New-Item -ItemType Directory -Force -Path $YamlBuild | Out-Null
    
    # Fix CMakeLists.txt if it has old cmake_minimum_required
    $YamlCmakeLists = Join-Path $YamlSrc "CMakeLists.txt"
    if (Test-Path $YamlCmakeLists) {
        $content = Get-Content $YamlCmakeLists -Raw
        if ($content -match 'cmake_minimum_required\s*\(VERSION\s+[<]?\s*3\.[0-4]') {
            Write-Host "Updating yaml-cpp CMakeLists.txt to require CMake 3.5+"
            $content = $content -replace 'cmake_minimum_required\s*\(VERSION\s+[<]?\s*3\.[0-4][^)]*\)', 'cmake_minimum_required(VERSION 3.5)'
            Set-Content $YamlCmakeLists -Value $content -NoNewline
        }
    }
    
    Push-Location $YamlBuild
    try {
        & cmake -G Ninja -S $YamlSrc -B $YamlBuild `
            -DBUILD_SHARED_LIBS=OFF `
            -DBUILD_TESTING=OFF `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_INSTALL_PREFIX="$InstallDir"
        & cmake --build $YamlBuild --config Release
        & cmake --install $YamlBuild --config Release
    } finally {
        Pop-Location
    }
}

Write-Host "Dependencies built successfully."
