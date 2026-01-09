# Download prebuilt nekobox_core from GitHub Releases
# Extracts nekobox_core.exe from the release zip file

param(
    [string]$DestDir = "",
    [string]$Version = "latest"
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

Write-Host "[INFO] Downloading prebuilt nekobox_core..."
Write-Host "[INFO] Destination: $DestDir"
Write-Host "[INFO] Version: $Version"

# Determine download URL
$RepoOwner = "MatsuriDayo"
$RepoName = "nekoray"
$CoreExeName = "nekobox_core.exe"
$TempDir = Join-Path $env:TEMP "nekoray_core_download"

# Clean up temp directory
if (Test-Path $TempDir) {
    Remove-Item -Path $TempDir -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $TempDir -Force | Out-Null

try {
    if ($Version -eq "latest") {
        # Get latest release
        Write-Host "[INFO] Fetching latest release info..."
        $ReleaseApiUrl = "https://api.github.com/repos/$RepoOwner/$RepoName/releases/latest"
        
        try {
            $ReleaseInfo = Invoke-RestMethod -Uri $ReleaseApiUrl -UseBasicParsing -ErrorAction Stop
            $Version = $ReleaseInfo.tag_name
            Write-Host "[INFO] Latest release: $Version"
        } catch {
            Write-Host "[WARN] Failed to fetch latest release info, trying direct download..." -ForegroundColor Yellow
            # Fallback: try to download from latest release page
            $ReleaseApiUrl = "https://api.github.com/repos/$RepoOwner/$RepoName/releases"
            $Releases = Invoke-RestMethod -Uri $ReleaseApiUrl -UseBasicParsing -ErrorAction Stop
            if ($Releases.Count -gt 0) {
                $Version = $Releases[0].tag_name
                Write-Host "[INFO] Using release: $Version"
            } else {
                throw "No releases found"
            }
        }
        
        # Find Windows release asset
        $WindowsAsset = $null
        foreach ($asset in $ReleaseInfo.assets) {
            if ($asset.name -like "*windows*" -and $asset.name -like "*.zip") {
                $WindowsAsset = $asset
                break
            }
        }
        
        if (-not $WindowsAsset) {
            throw "No Windows release zip found in latest release"
        }
        
        $DownloadUrl = $WindowsAsset.browser_download_url
        $ZipFileName = $WindowsAsset.name
    } else {
        # Specific version
        $ZipFileName = "nekoray-$Version-windows64.zip"
        $DownloadUrl = "https://github.com/$RepoOwner/$RepoName/releases/download/$Version/$ZipFileName"
    }
    
    $ZipPath = Join-Path $TempDir $ZipFileName
    
    Write-Host "[INFO] Downloading: $DownloadUrl"
    Write-Host "[INFO] Saving to: $ZipPath"
    
    # Download zip file
    Invoke-WebRequest -Uri $DownloadUrl -OutFile $ZipPath -UseBasicParsing -ErrorAction Stop
    
    Write-Host "[INFO] Extracting nekobox_core.exe from zip..."
    
    # Extract zip
    $ExtractDir = Join-Path $TempDir "extracted"
    Expand-Archive -Path $ZipPath -DestinationPath $ExtractDir -Force
    
    # Find nekobox_core.exe in extracted files
    $CoreExePath = Get-ChildItem -Path $ExtractDir -Filter $CoreExeName -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    
    if (-not $CoreExePath) {
        # Try alternative name
        $CoreExePath = Get-ChildItem -Path $ExtractDir -Filter "nekobox_core.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    }
    
    if (-not $CoreExePath) {
        throw "nekobox_core.exe not found in release zip"
    }
    
    # Copy to destination
    $DestExePath = Join-Path $DestDir $CoreExeName
    Copy-Item -Path $CoreExePath.FullName -Destination $DestExePath -Force
    
    Write-Host "[INFO] nekobox_core.exe downloaded successfully: $DestExePath"
    
} catch {
    Write-Host "[ERROR] Failed to download nekobox_core: $_" -ForegroundColor Red
    Write-Host "[INFO] You can manually download from: https://github.com/$RepoOwner/$RepoName/releases" -ForegroundColor Yellow
    Write-Host "[INFO] Extract nekobox_core.exe and place it in: $DestDir" -ForegroundColor Yellow
    exit 1
} finally {
    # Clean up temp directory
    if (Test-Path $TempDir) {
        Remove-Item -Path $TempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "[INFO] Core download completed."
