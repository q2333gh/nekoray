# Automated test script for Nekoray
# This script starts the application and performs basic tests

param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$WaitTime = 10,
    [switch]$SkipDownload = $false
)

$ErrorActionPreference = "Stop"

# Get repo root
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Nekoray Automated Test Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if executable exists
$FullExePath = Join-Path $RepoRoot $ExePath
if (-not (Test-Path $FullExePath)) {
    Write-Host "[ERROR] Executable not found: $FullExePath" -ForegroundColor Red
    Write-Host "[INFO] Attempting to build..." -ForegroundColor Yellow
    
    # Try to build
    $BuildScript = Join-Path $RepoRoot "build_windows.ps1"
    if (Test-Path $BuildScript) {
        & powershell -ExecutionPolicy Bypass -File $BuildScript -Config Release
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] Build failed" -ForegroundColor Red
            exit 1
        }
    } else {
        Write-Host "[ERROR] Build script not found" -ForegroundColor Red
        exit 1
    }
}

# Check if core exists
$CorePath = Join-Path (Split-Path $FullExePath -Parent) "nekobox_core.exe"
if (-not (Test-Path $CorePath) -and -not $SkipDownload) {
    Write-Host "[WARN] Core not found, attempting download..." -ForegroundColor Yellow
    $DownloadScript = Join-Path $RepoRoot "libs\download_core.ps1"
    if (Test-Path $DownloadScript) {
        $CoreDir = Split-Path $FullExePath -Parent
        & powershell -ExecutionPolicy Bypass -File $DownloadScript -DestDir $CoreDir
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] Core download failed" -ForegroundColor Red
            exit 1
        }
    }
}

Write-Host "[INFO] Starting application: $FullExePath" -ForegroundColor Green
Write-Host "[INFO] Waiting $WaitTime seconds for initialization..." -ForegroundColor Yellow
Write-Host ""

# Start the application
$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal

if ($null -eq $Process) {
    Write-Host "[ERROR] Failed to start application" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Process started with PID: $($Process.Id)" -ForegroundColor Green

# Wait for initialization
Start-Sleep -Seconds $WaitTime

# Check if process is still running
if ($Process.HasExited) {
    Write-Host "[ERROR] Application exited unexpectedly (Exit code: $($Process.ExitCode))" -ForegroundColor Red
    
    # Check for crash logs
    $CrashLog = Join-Path $RepoRoot "crash_log.txt"
    if (Test-Path $CrashLog) {
        Write-Host "[INFO] Crash log found:" -ForegroundColor Yellow
        Get-Content $CrashLog -Tail 20
    }
    
    exit 1
}

Write-Host "[INFO] Application is running" -ForegroundColor Green
Write-Host "[INFO] Process ID: $($Process.Id)" -ForegroundColor Green
Write-Host ""

# Check for log files
$LogFiles = @(
    "crash_log.txt",
    "core_not_found.log"
)

Write-Host "[INFO] Checking log files..." -ForegroundColor Yellow
foreach ($LogFile in $LogFiles) {
    $LogPath = Join-Path $RepoRoot $LogFile
    if (Test-Path $LogPath) {
        Write-Host "[WARN] Log file found: $LogFile" -ForegroundColor Yellow
        $Content = Get-Content $LogPath -Tail 10
        if ($Content) {
            Write-Host "  Last 10 lines:" -ForegroundColor Gray
            $Content | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
        }
    }
}

Write-Host ""
Write-Host "[INFO] Test completed successfully" -ForegroundColor Green
Write-Host "[INFO] Application is still running (PID: $($Process.Id))" -ForegroundColor Green
Write-Host "[INFO] You can now interact with the application manually" -ForegroundColor Yellow
Write-Host "[INFO] Press Ctrl+C to stop monitoring, or close the application window" -ForegroundColor Yellow
Write-Host ""

# Monitor the process
try {
    while (-not $Process.HasExited) {
        Start-Sleep -Seconds 5
        
        # Check for new errors in crash log
        $CrashLog = Join-Path $RepoRoot "crash_log.txt"
        if (Test-Path $CrashLog) {
            $LastWrite = (Get-Item $CrashLog).LastWriteTime
            if ($LastWrite -gt (Get-Date).AddSeconds(-10)) {
                Write-Host "[WARN] Recent activity in crash_log.txt" -ForegroundColor Yellow
            }
        }
    }
    
    Write-Host ""
    Write-Host "[INFO] Application exited (Exit code: $($Process.ExitCode))" -ForegroundColor Yellow
    
    # Check exit code
    if ($Process.ExitCode -ne 0) {
        Write-Host "[ERROR] Application exited with error code: $($Process.ExitCode)" -ForegroundColor Red
        
        # Show crash log if available
        $CrashLog = Join-Path $RepoRoot "crash_log.txt"
        if (Test-Path $CrashLog) {
            Write-Host "[INFO] Crash log:" -ForegroundColor Yellow
            Get-Content $CrashLog -Tail 30
        }
        
        exit $Process.ExitCode
    }
} catch {
    Write-Host "[ERROR] Monitoring error: $_" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Test completed" -ForegroundColor Green
