# PowerShell wrapper for WinAppDriver test
# Installs dependencies and runs the Python test

param(
    [string]$ExePath = "build\Release\nekobox.exe"
)

$ErrorActionPreference = "Continue"
$RepoRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "WinAppDriver Test Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check Python
Write-Host "[1/4] Checking Python..." -ForegroundColor Yellow
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command python3 -ErrorAction SilentlyContinue
}
if (-not $python) {
    Write-Host "[ERROR] Python not found. Please install Python 3.7+" -ForegroundColor Red
    Write-Host "  Download from: https://www.python.org/downloads/" -ForegroundColor Yellow
    exit 1
}
Write-Host "[OK] Python found: $($python.Source)" -ForegroundColor Green

# Check/Install selenium
Write-Host "[2/4] Checking selenium..." -ForegroundColor Yellow
$seleniumInstalled = & $python.Source -c "import selenium" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[INFO] Installing selenium..." -ForegroundColor Yellow
    & $python.Source -m pip install selenium --quiet
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Failed to install selenium" -ForegroundColor Red
        exit 1
    }
}
Write-Host "[OK] selenium installed" -ForegroundColor Green

# Check WinAppDriver
Write-Host "[3/4] Checking WinAppDriver..." -ForegroundColor Yellow
$wadPaths = @(
    "C:\Program Files (x86)\Windows Application Driver\WinAppDriver.exe",
    "C:\Program Files\Windows Application Driver\WinAppDriver.exe",
    "$env:LOCALAPPDATA\Programs\Windows Application Driver\WinAppDriver.exe"
)
$wadFound = $false
foreach ($path in $wadPaths) {
    if (Test-Path $path) {
        Write-Host "[OK] WinAppDriver found: $path" -ForegroundColor Green
        $wadFound = $true
        break
    }
}
if (-not $wadFound) {
    Write-Host "[WARN] WinAppDriver not found" -ForegroundColor Yellow
    Write-Host "[INFO] Please install from:" -ForegroundColor Yellow
    Write-Host "  https://github.com/microsoft/WinAppDriver/releases" -ForegroundColor White
    Write-Host "[INFO] Or run: winget install Microsoft.WinAppDriver" -ForegroundColor White
    $install = Read-Host "Continue anyway? (y/n)"
    if ($install -ne "y") {
        exit 1
    }
}

# Update Python script with correct path
Write-Host "[4/4] Preparing test script..." -ForegroundColor Yellow
$pythonScript = Join-Path $RepoRoot "test_winappdriver.py"
$fullExePath = Join-Path $RepoRoot $ExePath
if (-not (Test-Path $fullExePath)) {
    Write-Host "[ERROR] Executable not found: $fullExePath" -ForegroundColor Red
    exit 1
}

# Update APP_PATH in Python script
$scriptContent = Get-Content $pythonScript -Raw
$scriptContent = $scriptContent -replace 'APP_PATH = r"build\\Release\\nekobox.exe"', "APP_PATH = r'$($fullExePath.Replace('\', '\\'))'"
Set-Content $pythonScript -Value $scriptContent -NoNewline
Write-Host "[OK] Test script prepared" -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Running WinAppDriver Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Run test
& $python.Source $pythonScript
exit $LASTEXITCODE
