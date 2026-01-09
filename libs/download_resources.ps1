# Download geosite, geodb, and other public resources for Windows build
# Similar to libs/build_public_res.sh but for PowerShell

param(
    [string]$DestDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $DestDir -or $DestDir.Trim() -eq "") {
    $DestDir = Join-Path $PSScriptRoot "..\build\Release"
}

if (-not (Test-Path $DestDir)) {
    New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
}

Write-Host "[INFO] Downloading public resources to: $DestDir"

# Download geodata files
$geodataUrls = @{
    "geoip.dat" = "https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/geoip.dat"
    "geosite.dat" = "https://github.com/v2fly/domain-list-community/releases/latest/download/dlc.dat"
    "geoip.db" = "https://github.com/SagerNet/sing-geoip/releases/latest/download/geoip.db"
    "geosite.db" = "https://github.com/SagerNet/sing-geosite/releases/latest/download/geosite.db"
}

foreach ($file in $geodataUrls.Keys) {
    $url = $geodataUrls[$file]
    $destPath = Join-Path $DestDir $file
    
    Write-Host "[INFO] Downloading $file..."
    try {
        Invoke-WebRequest -Uri $url -OutFile $destPath -UseBasicParsing -ErrorAction Stop
        Write-Host "[INFO] Successfully downloaded $file"
    } catch {
        Write-Host "[ERROR] Failed to download $file from $url" -ForegroundColor Red
        Write-Host "Error: $_" -ForegroundColor Red
        exit 1
    }
}

# Copy res/public files if they exist
$publicResDir = Join-Path $PSScriptRoot "..\res\public"
if (Test-Path $publicResDir) {
    Write-Host "[INFO] Copying res/public files..."
    Get-ChildItem -Path $publicResDir -File | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $DestDir -Force
        Write-Host "[INFO] Copied $($_.Name)"
    }
} else {
    Write-Host "[WARN] res/public directory not found, skipping" -ForegroundColor Yellow
}

Write-Host "[INFO] Public resources download completed."
