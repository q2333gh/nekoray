<#
  Simple Windows local build script for nekoray (PowerShell, ASCII only)

  Usage (run from repo root):
    pwsh ./build_windows.ps1
    or
    powershell -ExecutionPolicy Bypass -File .\build_windows.ps1

  Parameters:
    -QtRoot           Qt SDK root directory (default: .\qt_lib\qt650)
    -Config           CMake configuration (Debug / Release / RelWithDebInfo / MinSizeRel)
    -DepsRoot         Prebuilt dependency root (default: .\libs\deps\built or .\libs\deps\package)
    -DisableGrpc      Disable gRPC and skip protobuf build
    -DepsBuildTimeoutSec Timeout for dependency builds in seconds (default: 120)
    -DownloadCore     Download prebuilt nekobox_core from GitHub Releases
    -DownloadResources Download geosite/geodb resources (default: true)

  Steps:
    1. Configure CMake with Qt6 and prebuilt external deps
    2. Build nekobox (MSVC)
    3. Copy build\<Config>\nekobox.exe to repo root as nekobox.exe
    4. Download public resources (geosite.dat, geosite.db, geoip.dat, geoip.db)
    5. Optionally download prebuilt nekobox_core if -DownloadCore is specified
#>

param(
    [string]$QtRoot = "",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [string]$DepsRoot = "",
    [switch]$DisableGrpc = $false,
    [int]$DepsBuildTimeoutSec = 600,
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
    return (Test-AnyPath -Paths $ProtocPaths) -and (Test-AnyPath -Paths $LibPaths) -and (Test-AnyPath -Paths $CmakePaths)
}

function Test-AnyPath {
    param([string[]]$Paths)
    foreach ($p in $Paths) {
        if (Test-Path -LiteralPath $p) {
            return $true
        }
    }
    return $false
}

function Invoke-ProcessWithTimeout {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [int]$TimeoutSec
    )
    $Proc = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -PassThru -NoNewWindow
    if (-not $Proc.WaitForExit($TimeoutSec * 1000)) {
        try { $Proc.Kill() } catch {}
        return $false
    }
    return $Proc.ExitCode -eq 0
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

$Ninja = Get-Command ninja -ErrorAction SilentlyContinue
$IsNinja = $null -ne $Ninja
$Generator = ""
if ($IsNinja) {
    $Generator = "Ninja Multi-Config"
    Write-Info "Using generator: $Generator"
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
        Write-Info "Ninja detected but Visual Studio dev environment not found; falling back to Visual Studio generator."
        $IsNinja = $false
        $Generator = ""
    }
}

$IsNinja = $IsNinja -and $script:UseVsDevCmd
if ($IsNinja) {
    Import-VsDevCmdEnv -VsDevCmd $script:VsDevCmd
}

$BuildDir = Join-Path $RepoRoot $(if ($IsNinja) { "build_ninja" } else { "build" })
$DefaultDepsRoot = Join-Path $RepoRoot "libs\deps\built"
if (-not $DepsRoot -or $DepsRoot.Trim() -eq "") {
    $PackageRoot = Join-Path $RepoRoot "libs\deps\package"
    if (Test-Path -LiteralPath $PackageRoot) {
        $DepsRoot = $PackageRoot
    } else {
        $DepsRoot = $DefaultDepsRoot
    }
}

if (-not (Test-Path -LiteralPath $DepsRoot)) {
    Write-ErrorMsg "DepsRoot '$DepsRoot' does not exist. Please set -DepsRoot to a valid prebuilt dependency directory."
    exit 1
}

Write-Info "Using DepsRoot: $DepsRoot"
$YamlConfigs = @(
    (Join-Path $DepsRoot "lib\cmake\yaml-cpp\yaml-cpp-config.cmake"),
    (Join-Path $DepsRoot "lib\cmake\yaml-cpp\yaml-cppConfig.cmake"),
    (Join-Path $DepsRoot "cmake\yaml-cpp\yaml-cpp-config.cmake"),
    (Join-Path $DepsRoot "cmake\yaml-cpp\yaml-cppConfig.cmake"),
    (Join-Path $DepsRoot "share\cmake\yaml-cpp\yaml-cpp-config.cmake"),
    (Join-Path $DepsRoot "share\cmake\yaml-cpp\yaml-cppConfig.cmake"),
    (Join-Path $DepsRoot "share\yaml-cpp\yaml-cpp-config.cmake"),
    (Join-Path $DepsRoot "share\yaml-cpp\yaml-cppConfig.cmake")
)
$ZxingConfigs = @(
    (Join-Path $DepsRoot "lib\cmake\ZXing\ZXingConfig.cmake"),
    (Join-Path $DepsRoot "lib\cmake\ZXing\ZXing-config.cmake"),
    (Join-Path $DepsRoot "cmake\ZXing\ZXingConfig.cmake"),
    (Join-Path $DepsRoot "cmake\ZXing\ZXing-config.cmake"),
    (Join-Path $DepsRoot "share\cmake\ZXing\ZXingConfig.cmake"),
    (Join-Path $DepsRoot "share\cmake\ZXing\ZXing-config.cmake"),
    (Join-Path $DepsRoot "share\cmake\zxing-cpp\zxing-cpp-config.cmake"),
    (Join-Path $DepsRoot "share\zxing-cpp\zxing-cpp-config.cmake"),
    (Join-Path $DepsRoot "share\ZXing\ZXingConfig.cmake")
)
if (-not (Test-AnyPath -Paths $YamlConfigs)) {
    if (([IO.Path]::GetFullPath($DepsRoot)) -eq ([IO.Path]::GetFullPath($DefaultDepsRoot))) {
        $BuildDepsScript = Join-Path $RepoRoot "libs\build_deps_windows.ps1"
        if (Test-Path -LiteralPath $BuildDepsScript) {
            Write-Info "yaml-cpp missing; building dependencies..."
            $Ok = Invoke-ProcessWithTimeout -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", $BuildDepsScript) -TimeoutSec $DepsBuildTimeoutSec
            if (-not $Ok) {
                Write-ErrorMsg "Dependency build timed out after ${DepsBuildTimeoutSec}s."
                exit 1
            }
        } else {
            Write-ErrorMsg "yaml-cpp config not found in DepsRoot '$DepsRoot' and build_deps_windows.ps1 is missing."
            exit 1
        }
    } else {
        Write-ErrorMsg "yaml-cpp config not found in DepsRoot '$DepsRoot'. Provide a prebuilt deps cache with yaml-cpp."
        exit 1
    }
}
if (-not (Test-AnyPath -Paths $ZxingConfigs)) {
    if (([IO.Path]::GetFullPath($DepsRoot)) -eq ([IO.Path]::GetFullPath($DefaultDepsRoot))) {
        $BuildDepsScript = Join-Path $RepoRoot "libs\build_deps_windows.ps1"
        if (Test-Path -LiteralPath $BuildDepsScript) {
            Write-Info "ZXing missing; building dependencies..."
            $Ok = Invoke-ProcessWithTimeout -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", $BuildDepsScript) -TimeoutSec $DepsBuildTimeoutSec
            if (-not $Ok) {
                Write-ErrorMsg "Dependency build timed out after ${DepsBuildTimeoutSec}s."
                exit 1
            }
        } else {
            Write-ErrorMsg "ZXing config not found in DepsRoot '$DepsRoot' and build_deps_windows.ps1 is missing."
            exit 1
        }
    } else {
        Write-ErrorMsg "ZXing config not found in DepsRoot '$DepsRoot'. Provide a prebuilt deps cache with ZXing."
        exit 1
    }
}

# Build Protobuf if gRPC is enabled
if (-not $DisableGrpc) {
    Write-Info "Building Protobuf dependency for gRPC..."
    if (Test-ProtobufInstall -InstallDir $DepsRoot) {
        Write-Info "Using cached protobuf from $DepsRoot"
    } else {
        $ProtobufScript = Join-Path $RepoRoot "libs\build_protobuf_windows.ps1"
        if (([IO.Path]::GetFullPath($DepsRoot)) -ne ([IO.Path]::GetFullPath($DefaultDepsRoot))) {
            Write-ErrorMsg "Protobuf not found in DepsRoot '$DepsRoot'. Populate it with prebuilt deps or use -DepsRoot '$DefaultDepsRoot'."
            exit 1
        }
        if (Test-Path $ProtobufScript) {
            $Ok = Invoke-ProcessWithTimeout -FilePath "powershell" -ArgumentList @("-ExecutionPolicy", "Bypass", "-File", $ProtobufScript) -TimeoutSec $DepsBuildTimeoutSec
            if (-not $Ok) {
                Write-Warning "Protobuf build failed, but continuing..."
            }
        } else {
            Write-Warning "Protobuf build script not found: $ProtobufScript"
        }
    }
} else {
    Write-Info "gRPC disabled (skip protobuf build)."
}

Write-Info "Configuring CMake (Config=$Config, QT_VERSION_MAJOR=6, gRPC enabled)..."

$CmakeArgs = @(
    "-S", $RepoRoot,
    "-B", $BuildDir,
    "-DQT_VERSION_MAJOR=6",
    "-DNKR_LIBS=$DepsRoot",
    "-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false",
    "-DCMAKE_PREFIX_PATH=$QtRoot"
)
if ($DisableGrpc) {
    $CmakeArgs += "-DNKR_NO_GRPC=ON"
}
if ($Generator) {
    $CmakeArgs = @("-G", $Generator) + $CmakeArgs
}
Invoke-CMake -CmakeArgs $CmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-ErrorMsg "CMake configure failed (exit code=$LASTEXITCODE)."
    exit $LASTEXITCODE
}

Write-Info "Building (Config=$Config)..."

if ($IsNinja) {
    Invoke-CMake -CmakeArgs @("--build", $BuildDir, "--config", $Config)
} else {
    Invoke-CMake -CmakeArgs @("--build", $BuildDir, "--config", $Config, "--", "/m:1", "/p:TrackFileAccess=false", "/p:EnableMinimalRebuild=false")
}

if ($LASTEXITCODE -ne 0) {
    Write-ErrorMsg "Build failed (exit code=$LASTEXITCODE)."
    exit $LASTEXITCODE
}

$BuiltExe = Join-Path $BuildDir "$Config\nekobox.exe"
if ($IsNinja -and -not (Test-Path -LiteralPath $BuiltExe)) {
    $BuiltExe = Join-Path $BuildDir "nekobox.exe"
}

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
    if ($IsNinja -and -not (Test-Path -LiteralPath $ReleaseDir)) {
        $ReleaseDir = $BuildDir
    }

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
    if ($IsNinja -and -not (Test-Path -LiteralPath $ReleaseDir)) {
        $ReleaseDir = $BuildDir
    }

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



