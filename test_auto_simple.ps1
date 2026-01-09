# Simple automated test script for Nekoray
# Just starts the app and monitors it

param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$WaitTime = 20
)

$ErrorActionPreference = "Continue"

# Get script directory and repo root (script is in repo root)
$RepoRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Nekoray Simple Auto Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "[INFO] Repo root: $RepoRoot" -ForegroundColor Gray
Write-Host "[INFO] Exe path: $ExePath" -ForegroundColor Gray
Write-Host ""

$FullExePath = if ([System.IO.Path]::IsPathRooted($ExePath)) {
    $ExePath
} else {
    Join-Path $RepoRoot $ExePath
}
Write-Host "[INFO] Full exe path: $FullExePath" -ForegroundColor Gray
Write-Host ""

if (-not (Test-Path $FullExePath)) {
    Write-Host "[ERROR] Executable not found: $FullExePath" -ForegroundColor Red
    exit 1
}

Write-Host "[INFO] Starting: $FullExePath" -ForegroundColor Green
Write-Host "[INFO] Will monitor for $WaitTime seconds..." -ForegroundColor Yellow
Write-Host ""

# Start process
$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal

if ($null -eq $Process) {
    Write-Host "[ERROR] Failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "[OK] Started (PID: $($Process.Id))" -ForegroundColor Green
Write-Host ""

# Monitor
$StartTime = Get-Date
$CheckInterval = 2

while (-not $Process.HasExited) {
    $Elapsed = ((Get-Date) - $StartTime).TotalSeconds
    
    if ($Elapsed -ge $WaitTime) {
        Write-Host "[INFO] Monitoring time reached ($WaitTime seconds)" -ForegroundColor Yellow
        break
    }
    
    Start-Sleep -Seconds $CheckInterval
    
    # Check logs every 5 seconds
    if ([math]::Floor($Elapsed) % 5 -eq 0) {
        $CrashLog = Join-Path $RepoRoot "crash_log.txt"
        if (Test-Path $CrashLog) {
            $LastWrite = (Get-Item $CrashLog).LastWriteTime
            if ($LastWrite -gt $StartTime) {
                Write-Host "[LOG] Recent activity in crash_log.txt" -ForegroundColor Gray
            }
        }
    }
}

if ($Process.HasExited) {
    Write-Host ""
    Write-Host "[INFO] Application exited (Code: $($Process.ExitCode))" -ForegroundColor Yellow
    
    if ($Process.ExitCode -ne 0) {
        Write-Host "[ERROR] Non-zero exit code!" -ForegroundColor Red
        
        $CrashLog = Join-Path $RepoRoot "crash_log.txt"
        if (Test-Path $CrashLog) {
            Write-Host "[INFO] Crash log (last 20 lines):" -ForegroundColor Yellow
            Get-Content $CrashLog -Tail 20
        }
        
        exit $Process.ExitCode
    }
} else {
    Write-Host ""
    Write-Host "[OK] Application still running after $WaitTime seconds" -ForegroundColor Green
    Write-Host "[INFO] PID: $($Process.Id)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "[INFO] Test completed" -ForegroundColor Green
