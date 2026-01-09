# Automated test and run script for Nekoray
# Starts the app, monitors it, and checks logs

param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$InitWait = 15,
    [int]$MonitorTime = 60,
    [switch]$AutoBuild = $false,
    [switch]$AutoDownloadCore = $false
)

$ErrorActionPreference = "Continue"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

function Write-Info { param([string]$msg) Write-Host "[INFO] $msg" -ForegroundColor Green }
function Write-Warn { param([string]$msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Error { param([string]$msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Step { param([string]$msg) Write-Host "[STEP] $msg" -ForegroundColor Cyan }

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Nekoray Auto Test & Run" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Check/Build executable
$FullExePath = Join-Path $RepoRoot $ExePath
if (-not (Test-Path $FullExePath)) {
    Write-Warn "Executable not found: $FullExePath"
    
    if ($AutoBuild) {
        Write-Step "Building application..."
        $BuildScript = Join-Path $RepoRoot "build_windows.ps1"
        if (Test-Path $BuildScript) {
            & powershell -ExecutionPolicy Bypass -File $BuildScript -Config Release
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Build failed"
                exit 1
            }
            Write-Info "Build completed"
        } else {
            Write-Error "Build script not found"
            exit 1
        }
    } else {
        Write-Error "Executable not found. Use -AutoBuild to build automatically."
        exit 1
    }
}

# Step 2: Check/Download core
$CorePath = Join-Path (Split-Path $FullExePath -Parent) "nekobox_core.exe"
if (-not (Test-Path $CorePath)) {
    Write-Warn "Core not found: $CorePath"
    
    if ($AutoDownloadCore) {
        Write-Step "Downloading core..."
        $DownloadScript = Join-Path $RepoRoot "libs\download_core.ps1"
        if (Test-Path $DownloadScript) {
            $CoreDir = Split-Path $FullExePath -Parent
            & powershell -ExecutionPolicy Bypass -File $DownloadScript -DestDir $CoreDir
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Core download failed"
                exit 1
            }
            Write-Info "Core downloaded"
        } else {
            Write-Error "Download script not found"
            exit 1
        }
    } else {
        Write-Warn "Core not found. Use -AutoDownloadCore to download automatically."
    }
}

# Step 3: Clean old logs
Write-Step "Cleaning old log files..."
$LogFiles = @("crash_log.txt", "core_not_found.log")
foreach ($LogFile in $LogFiles) {
    $LogPath = Join-Path $RepoRoot $LogFile
    if (Test-Path $LogPath) {
        Remove-Item $LogPath -Force -ErrorAction SilentlyContinue
        Write-Info "Removed old log: $LogFile"
    }
}

# Step 4: Start application
Write-Step "Starting application..."
Write-Info "Path: $FullExePath"
Write-Info "Waiting $InitWait seconds for initialization..."

$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal -ArgumentList "-many"

if ($null -eq $Process) {
    Write-Error "Failed to start application"
    exit 1
}

Write-Info "Process started (PID: $($Process.Id))"
Write-Host ""

# Step 5: Wait for initialization
$InitStart = Get-Date
$InitEnd = $InitStart.AddSeconds($InitWait)

while ((Get-Date) -lt $InitEnd -and -not $Process.HasExited) {
    $Remaining = [math]::Ceiling(($InitEnd - (Get-Date)).TotalSeconds)
    if ($Remaining % 5 -eq 0 -and $Remaining -gt 0) {
        Write-Host "  Initializing... ($Remaining seconds remaining)" -ForegroundColor Gray
    }
    Start-Sleep -Seconds 1
}

if ($Process.HasExited) {
    Write-Error "Application exited during initialization (Code: $($Process.ExitCode))"
    
    # Show crash log
    $CrashLog = Join-Path $RepoRoot "crash_log.txt"
    if (Test-Path $CrashLog) {
        Write-Host ""
        Write-Warn "Crash log:"
        Get-Content $CrashLog | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
    
    exit 1
}

Write-Info "Initialization completed"
Write-Host ""

# Step 6: Check logs
Write-Step "Checking logs..."
$CrashLog = Join-Path $RepoRoot "crash_log.txt"
if (Test-Path $CrashLog) {
    $LogContent = Get-Content $CrashLog
    if ($LogContent) {
        Write-Info "Found crash_log.txt ($($LogContent.Count) lines)"
        Write-Host "  Recent entries:" -ForegroundColor Gray
        $LogContent | Select-Object -Last 10 | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
    }
} else {
    Write-Info "No crash_log.txt found (this is good)"
}

# Step 7: Monitor
Write-Host ""
Write-Step "Monitoring application for $MonitorTime seconds..."
Write-Info "Application is running (PID: $($Process.Id))"
Write-Info "You can interact with it now"
Write-Host ""

$MonitorStart = Get-Date
$MonitorEnd = $MonitorStart.AddSeconds($MonitorTime)
$LastLogCheck = $MonitorStart
$LogCheckInterval = 5

while ((Get-Date) -lt $MonitorEnd -and -not $Process.HasExited) {
    $Elapsed = ((Get-Date) - $MonitorStart).TotalSeconds
    $Remaining = [math]::Ceiling(($MonitorEnd - (Get-Date)).TotalSeconds)
    
    # Check logs periodically
    if (((Get-Date) - $LastLogCheck).TotalSeconds -ge $LogCheckInterval) {
        $LastLogCheck = Get-Date
        
        if (Test-Path $CrashLog) {
            $LogInfo = Get-Item $CrashLog
            if ($LogInfo.LastWriteTime -gt $MonitorStart) {
                $NewLines = Get-Content $CrashLog | Select-Object -Last 3
                if ($NewLines) {
                    Write-Host "[LOG] New entries:" -ForegroundColor Gray
                    $NewLines | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
                }
            }
        }
    }
    
    # Show progress every 10 seconds
    if ([math]::Floor($Elapsed) % 10 -eq 0 -and $Elapsed -gt 0) {
        Write-Host "[MONITOR] Running... ($Remaining seconds remaining)" -ForegroundColor Gray
    }
    
    Start-Sleep -Seconds 1
}

# Step 8: Final status
Write-Host ""
Write-Step "Test completed"

if ($Process.HasExited) {
    Write-Warn "Application exited (Code: $($Process.ExitCode))"
    
    if ($Process.ExitCode -ne 0) {
        Write-Error "Non-zero exit code indicates an error"
        
        # Show final crash log
        if (Test-Path $CrashLog) {
            Write-Host ""
            Write-Warn "Final crash log (last 20 lines):"
            Get-Content $CrashLog -Tail 20 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        }
        
        exit $Process.ExitCode
    }
} else {
    Write-Info "Application is still running (PID: $($Process.Id))"
    Write-Info "Monitoring period completed"
}

# Final log summary
Write-Host ""
Write-Step "Log Summary"
if (Test-Path $CrashLog) {
    $LogContent = Get-Content $CrashLog
    Write-Info "crash_log.txt: $($LogContent.Count) lines"
    if ($LogContent.Count -gt 0) {
        Write-Host "  Last 5 lines:" -ForegroundColor Gray
        $LogContent | Select-Object -Last 5 | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
    }
} else {
    Write-Info "No crash_log.txt (no errors detected)"
}

Write-Host ""
Write-Info "Test script completed successfully"
Write-Info "Application PID: $($Process.Id)"
