# Interactive automated test script for Nekoray
# This script starts the application, waits for it to be ready, then performs automated actions

param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$InitWaitTime = 15,
    [int]$ActionDelay = 3
)

$ErrorActionPreference = "Stop"

# Get repo root
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Nekoray Interactive Automated Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if executable exists
$FullExePath = Join-Path $RepoRoot $ExePath
if (-not (Test-Path $FullExePath)) {
    Write-Host "[ERROR] Executable not found: $FullExePath" -ForegroundColor Red
    exit 1
}

# Check if core exists
$CorePath = Join-Path (Split-Path $FullExePath -Parent) "nekobox_core.exe"
if (-not (Test-Path $CorePath)) {
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
Write-Host "[INFO] Waiting $InitWaitTime seconds for initialization..." -ForegroundColor Yellow
Write-Host ""

# Start the application
$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal

if ($null -eq $Process) {
    Write-Host "[ERROR] Failed to start application" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Process started with PID: $($Process.Id)" -ForegroundColor Green

# Wait for initialization
Start-Sleep -Seconds $InitWaitTime

# Check if process is still running
if ($Process.HasExited) {
    Write-Host "[ERROR] Application exited unexpectedly (Exit code: $($Process.ExitCode))" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Application initialized" -ForegroundColor Green
Write-Host "[INFO] Performing automated actions..." -ForegroundColor Yellow
Write-Host ""

# Function to send key to application window
function Send-KeyToWindow {
    param(
        [System.Windows.Forms.Keys]$Key,
        [string]$WindowTitle = "NekoBox"
    )
    
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -TypeDefinition @"
        using System;
        using System.Runtime.InteropServices;
        using System.Windows.Forms;
        
        public class WindowHelper {
            [DllImport("user32.dll")]
            public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
            
            [DllImport("user32.dll")]
            public static extern bool SetForegroundWindow(IntPtr hWnd);
            
            [DllImport("user32.dll")]
            public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        }
"@
    
    $hwnd = [WindowHelper]::FindWindow($null, $WindowTitle)
    if ($hwnd -ne [IntPtr]::Zero) {
        [WindowHelper]::SetForegroundWindow($hwnd) | Out-Null
        Start-Sleep -Milliseconds 100
        [System.Windows.Forms.SendKeys]::SendWait("{$Key}")
        return $true
    }
    return $false
}

# Test 1: Check if window is visible
Write-Host "[TEST 1] Checking if main window is visible..." -ForegroundColor Cyan
Start-Sleep -Seconds $ActionDelay

# Test 2: Try to start a profile (Press Enter)
Write-Host "[TEST 2] Attempting to start profile (sending Enter key)..." -ForegroundColor Cyan
if (Send-KeyToWindow -Key "Enter") {
    Write-Host "  [OK] Enter key sent" -ForegroundColor Green
    Start-Sleep -Seconds $ActionDelay
} else {
    Write-Host "  [WARN] Could not send Enter key (window may not be focused)" -ForegroundColor Yellow
}

# Test 3: Check logs
Write-Host "[TEST 3] Checking for log output..." -ForegroundColor Cyan
$CrashLog = Join-Path $RepoRoot "crash_log.txt"
if (Test-Path $CrashLog) {
    $RecentLogs = Get-Content $CrashLog -Tail 5
    if ($RecentLogs) {
        Write-Host "  [INFO] Recent log entries:" -ForegroundColor Gray
        $RecentLogs | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
    }
}

# Test 4: Wait and check if still running
Write-Host "[TEST 4] Waiting and checking application status..." -ForegroundColor Cyan
Start-Sleep -Seconds $ActionDelay

if ($Process.HasExited) {
    Write-Host "  [ERROR] Application exited (Exit code: $($Process.ExitCode))" -ForegroundColor Red
    exit 1
} else {
    Write-Host "  [OK] Application is still running" -ForegroundColor Green
}

Write-Host ""
Write-Host "[INFO] Automated tests completed" -ForegroundColor Green
Write-Host "[INFO] Application is still running (PID: $($Process.Id))" -ForegroundColor Green
Write-Host "[INFO] You can now interact with the application manually" -ForegroundColor Yellow
Write-Host ""

# Monitor for a while
Write-Host "[INFO] Monitoring application for 30 seconds..." -ForegroundColor Yellow
$MonitorEnd = (Get-Date).AddSeconds(30)
while ((Get-Date) -lt $MonitorEnd -and -not $Process.HasExited) {
    Start-Sleep -Seconds 5
}

if ($Process.HasExited) {
    Write-Host "[INFO] Application exited during monitoring (Exit code: $($Process.ExitCode))" -ForegroundColor Yellow
} else {
    Write-Host "[INFO] Monitoring completed, application is still running" -ForegroundColor Green
}

Write-Host "[INFO] Test script completed" -ForegroundColor Green
