# Build nekobox_core for Windows (PowerShell version of libs/build_go.sh)
# Requires: Go 1.22+, git, and source repositories (sing-box, sing-quic, libneko)

param(
    [string]$DestDir = "",
    [string]$Version = ""
)

$ErrorActionPreference = "Stop"

# Get repo root
$RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $RepoRoot

if (-not $DestDir -or $DestDir.Trim() -eq "") {
    $DestDir = Join-Path $RepoRoot "build\Release"
}

if (-not (Test-Path $DestDir)) {
    New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
}

if (-not $Version -or $Version.Trim() -eq "") {
    $VersionFile = Join-Path $RepoRoot "nekoray_version.txt"
    if (Test-Path $VersionFile) {
        $Version = (Get-Content $VersionFile -Raw).Trim()
    } else {
        $Version = "unknown"
    }
}

$VersionStandalone = "nekoray-$Version"

Write-Host "[INFO] Building nekobox_core for Windows (amd64)"
Write-Host "[INFO] Destination: $DestDir"
Write-Host "[INFO] Version: $VersionStandalone"

# Check Go
try {
    $goVersion = & go version 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $goVersion) {
        throw "Go not found"
    }
    Write-Host "[INFO] Go OK: $goVersion"
} catch {
    Write-Host "[ERROR] Go not found. Please install Go 1.22+ and ensure it is in PATH." -ForegroundColor Red
    Write-Host "[INFO] Download from: https://go.dev/dl/" -ForegroundColor Yellow
    exit 1
}

# Check if source repos exist
$SourceDir = Split-Path -Parent $RepoRoot
$RequiredRepos = @("sing-box", "sing-quic", "libneko")

$MissingRepos = @()
foreach ($repo in $RequiredRepos) {
    $repoPath = Join-Path $SourceDir $repo
    if (-not (Test-Path $repoPath)) {
        $MissingRepos += $repo
    }
}

if ($MissingRepos.Count -gt 0) {
    Write-Host "[WARN] Missing source repositories: $($MissingRepos -join ', ')" -ForegroundColor Yellow
    Write-Host "[INFO] Run 'bash libs/get_source.sh' first to clone required repositories." -ForegroundColor Yellow
    Write-Host "[INFO] Or manually clone:" -ForegroundColor Yellow
    Write-Host "  git clone https://github.com/MatsuriDayo/sing-box.git" -ForegroundColor Yellow
    Write-Host "  git clone https://github.com/MatsuriDayo/sing-quic.git" -ForegroundColor Yellow
    Write-Host "  git clone https://github.com/MatsuriDayo/libneko.git" -ForegroundColor Yellow
    exit 1
}

# Set environment
$env:CGO_ENABLED = "0"
$env:GOOS = "windows"
$env:GOARCH = "amd64"

# Build nekobox_core
Write-Host "[INFO] Building nekobox_core..."
$CoreDir = Join-Path $RepoRoot "go\cmd\nekobox_core"

Push-Location $CoreDir
try {
    $ldflags = "-w -s -X github.com/matsuridayo/libneko/neko_common.Version_neko=$VersionStandalone"
    $tags = "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_ech"
    
    & go build -v -o $DestDir -trimpath -ldflags $ldflags -tags $tags
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Build failed (exit code=$LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    
    Write-Host "[INFO] nekobox_core built successfully: $DestDir\nekobox_core.exe"
} finally {
    Pop-Location
}

Write-Host "[INFO] Core build completed."
