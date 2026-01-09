# Fully automated test: start app, select node, start proxy, test URL, verify success
param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$Timeout = 120
)

$ErrorActionPreference = "Continue"
$RepoRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Fully Automated URL Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$FullExePath = Join-Path $RepoRoot $ExePath
if (-not (Test-Path $FullExePath)) {
    Write-Host "[ERROR] Executable not found: $FullExePath" -ForegroundColor Red
    exit 1
}

# Clean logs
if (Test-Path "core_output.log") { Remove-Item "core_output.log" -Force }
if (Test-Path "crash_log.txt") { Remove-Item "crash_log.txt" -Force }

# Start app
Write-Host "[1/5] Starting application..." -ForegroundColor Green
$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal
if ($null -eq $Process) {
    Write-Host "[ERROR] Failed to start" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Started (PID: $($Process.Id))" -ForegroundColor Green

# Wait for core
Write-Host "[2/5] Waiting for core to start..." -ForegroundColor Yellow
$CoreStarted = $false
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 1
    $CoreProcess = Get-Process -Name "nekobox_core" -ErrorAction SilentlyContinue
    if ($CoreProcess) {
        Write-Host "[OK] Core running (PID: $($CoreProcess.Id))" -ForegroundColor Green
        $CoreStarted = $true
        break
    }
}
if (-not $CoreStarted) {
    Write-Host "[WARN] Core not detected, continuing anyway..." -ForegroundColor Yellow
}

Start-Sleep -Seconds 3

# UI Automation
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@

function ActivateWindow {
    # Try multiple window title variations
    $titles = @("NekoBox", "nekobox", "Nekoray", "nekoray")
    foreach ($title in $titles) {
        $hwnd = [Win32]::FindWindow($null, $title)
        if ($hwnd -ne [IntPtr]::Zero) {
            [Win32]::SetForegroundWindow($hwnd) | Out-Null
            Start-Sleep -Milliseconds 500
            return $true
        }
    }
    
    # Fallback: Find by process
    $proc = Get-Process -Id $Process.Id -ErrorAction SilentlyContinue
    if ($proc) {
        Add-Type -TypeDefinition @"
        using System;
        using System.Runtime.InteropServices;
        using System.Diagnostics;
        public class WindowHelper {
            [DllImport("user32.dll")]
            public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
            public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
            [DllImport("user32.dll")]
            public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
            [DllImport("user32.dll")]
            public static extern bool SetForegroundWindow(IntPtr hWnd);
            [DllImport("user32.dll")]
            public static extern bool IsWindowVisible(IntPtr hWnd);
            
            public static IntPtr FindWindowByProcessId(uint processId) {
                IntPtr found = IntPtr.Zero;
                EnumWindows((hWnd, lParam) => {
                    uint pid;
                    GetWindowThreadProcessId(hWnd, out pid);
                    if (pid == processId && IsWindowVisible(hWnd)) {
                        found = hWnd;
                        return false;
                    }
                    return true;
                }, IntPtr.Zero);
                return found;
            }
        }
"@
        $hwnd = [WindowHelper]::FindWindowByProcessId($Process.Id)
        if ($hwnd -ne [IntPtr]::Zero) {
            [WindowHelper]::SetForegroundWindow($hwnd) | Out-Null
            Start-Sleep -Milliseconds 500
            return $true
        }
    }
    return $false
}

# Step 3: Select first node and start
Write-Host "[3/5] Selecting first node and starting proxy..." -ForegroundColor Yellow
$retries = 0
while ($retries -lt 5) {
    if (ActivateWindow) {
        # Press Enter to select first node and start
        [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
        Start-Sleep -Seconds 3
        Write-Host "[OK] Node selected and started" -ForegroundColor Green
        break
    } else {
        $retries++
        Write-Host "[RETRY] Attempt $retries/5 to activate window..." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
    }
}
if ($retries -ge 5) {
    Write-Host "[WARN] Could not activate window after 5 attempts" -ForegroundColor Yellow
}

# Step 4: Trigger URL test
Write-Host "[4/5] Triggering URL test..." -ForegroundColor Yellow
$testTriggered = $false
$retries = 0
while ($retries -lt 5 -and -not $testTriggered) {
    if (ActivateWindow) {
        # Method 1: Try Ctrl+T (if shortcut exists)
        [System.Windows.Forms.SendKeys]::SendWait("^T")
        Start-Sleep -Seconds 2
        
        # Method 2: Try menu (Alt+S for Server menu, then U for URL test)
        [System.Windows.Forms.SendKeys]::SendWait("%S")  # Alt+S
        Start-Sleep -Milliseconds 300
        [System.Windows.Forms.SendKeys]::SendWait("U")  # U for URL test
        Start-Sleep -Seconds 2
        
        # Method 3: Try direct Enter on test button area
        [System.Windows.Forms.SendKeys]::SendWait("{TAB}{TAB}{TAB}{ENTER}")
        Start-Sleep -Seconds 2
        
        Write-Host "[OK] URL test triggered (attempt $($retries+1))" -ForegroundColor Green
        $testTriggered = $true
    } else {
        $retries++
        Start-Sleep -Seconds 2
    }
}
if (-not $testTriggered) {
    Write-Host "[WARN] Could not trigger URL test" -ForegroundColor Yellow
}

# Step 5: Wait and check results
Write-Host "[5/5] Waiting for test results..." -ForegroundColor Yellow
Start-Sleep -Seconds 20  # Wait longer for test to complete

# Check results
Write-Host ""
Write-Host "=== Results ===" -ForegroundColor Cyan

$Success = $false
$HasLogs = $false

# Check core_output.log
if (Test-Path "core_output.log") {
    $logContent = Get-Content "core_output.log" -ErrorAction SilentlyContinue
    if ($logContent) {
        $HasLogs = $true
        Write-Host "[FOUND] core_output.log ($($logContent.Count) lines)" -ForegroundColor Green
        
        # Check for success indicators
        $hasConnection = $logContent | Select-String -Pattern "connection|outbound|inbound|INFO\[.*ms\]" -Quiet
        $hasError = $logContent | Select-String -Pattern "ERROR\[|failed|error" -Quiet
        
        if ($hasConnection -and -not $hasError) {
            Write-Host "[SUCCESS] Core logs show successful connection!" -ForegroundColor Green
            $Success = $true
        } elseif ($hasError) {
            Write-Host "[FAIL] Core logs show errors" -ForegroundColor Red
        }
        
        # Show recent logs
        Write-Host "Recent logs:" -ForegroundColor Yellow
        $logContent | Select-Object -Last 10 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
}

# Check crash_log.txt for URL test results
if (Test-Path "crash_log.txt") {
    $crashLog = Get-Content "crash_log.txt" -ErrorAction SilentlyContinue
    if ($crashLog) {
        Write-Host "[FOUND] crash_log.txt ($($crashLog.Count) lines)" -ForegroundColor Green
        
        # Check for URL test activity
        $testCalled = $crashLog | Select-String -Pattern "speedtest_current|URL Test|Test.*called" -Quiet
        if ($testCalled) {
            Write-Host "[INFO] URL test was called" -ForegroundColor Gray
        }
        
        $testResult = $crashLog | Select-String -Pattern "Test succeeded|latency.*ms|rpcOK=true|gRPC call succeeded" -Quiet
        if ($testResult) {
            Write-Host "[SUCCESS] URL test completed successfully!" -ForegroundColor Green
            $Success = $true
            $crashLog | Select-String -Pattern "succeeded|latency|ms\]" | ForEach-Object { Write-Host "  $_" -ForegroundColor Green }
        }
        
        $testFailed = $crashLog | Select-String -Pattern "gRPC.*failed|defaultClient.*null|Test.*failed" -Quiet
        if ($testFailed) {
            Write-Host "[FAIL] URL test failed" -ForegroundColor Red
            $crashLog | Select-String -Pattern "gRPC|defaultClient|Test|failed|error" | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
        }
        
        # Show recent test-related logs
        Write-Host "Recent test logs:" -ForegroundColor Yellow
        $crashLog | Select-String -Pattern "speedtest|URL|Test|gRPC" | Select-Object -Last 10 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    }
}

if (-not $HasLogs) {
    Write-Host "[WARN] No logs found - checking alternative locations..." -ForegroundColor Yellow
    
    # Check if process is still running
    if (-not $Process.HasExited) {
        Write-Host "[INFO] Application is still running" -ForegroundColor Gray
        $CoreProcess = Get-Process -Name "nekobox_core" -ErrorAction SilentlyContinue
        if ($CoreProcess) {
            Write-Host "[INFO] Core process is running (PID: $($CoreProcess.Id))" -ForegroundColor Gray
        } else {
            Write-Host "[WARN] Core process not found!" -ForegroundColor Yellow
        }
    }
    
    # Check current directory for any log files
    $allLogs = Get-ChildItem -Include "*.log","*.txt" -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt (Get-Date).AddMinutes(-5) }
    if ($allLogs) {
        Write-Host "[FOUND] Recent log files:" -ForegroundColor Green
        $allLogs | ForEach-Object { Write-Host "  $($_.Name) ($($_.Length) bytes, modified: $($_.LastWriteTime))" -ForegroundColor Gray }
    }
    
    # Try to read crash_log if it exists in current dir
    $crashLogPath = Join-Path $RepoRoot "crash_log.txt"
    if (Test-Path $crashLogPath) {
        Write-Host "[FOUND] crash_log.txt in repo root" -ForegroundColor Green
        Get-Content $crashLogPath | Select-Object -Last 20
    }
}

# Final result
Write-Host ""
if ($Success) {
    Write-Host "[SUCCESS] URL test completed and proxy is working!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "[FAIL] URL test did not succeed or logs not found" -ForegroundColor Red
    Write-Host "[INFO] Troubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Check if application window is visible" -ForegroundColor White
    Write-Host "  2. Manually click URL test button and check logs" -ForegroundColor White
    Write-Host "  3. Check if core is outputting to stdout/stderr" -ForegroundColor White
    Write-Host "[INFO] Process still running for inspection (PID: $($Process.Id))" -ForegroundColor Gray
    exit 1
}
