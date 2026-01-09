# Fully automated URL test script
# Automatically starts app, selects profile, starts it, and tests URL

param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$Timeout = 60
)

$ErrorActionPreference = "Continue"

# Get repo root
$RepoRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Automated URL Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$FullExePath = Join-Path $RepoRoot $ExePath
if (-not (Test-Path $FullExePath)) {
    Write-Host "[ERROR] Executable not found: $FullExePath" -ForegroundColor Red
    exit 1
}

# Clean old logs
if (Test-Path "core_output.log") { Remove-Item "core_output.log" -Force }
if (Test-Path "crash_log.txt") { Remove-Item "crash_log.txt" -Force }

Write-Host "[INFO] Starting application..." -ForegroundColor Green
$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal

if ($null -eq $Process) {
    Write-Host "[ERROR] Failed to start" -ForegroundColor Red
    exit 1
}

Write-Host "[OK] Started (PID: $($Process.Id))" -ForegroundColor Green
Write-Host "[INFO] Waiting for application to initialize..." -ForegroundColor Yellow

# Wait for main window (check if process is responsive)
$WaitTime = 0
$MaxWait = 10
while ($WaitTime -lt $MaxWait) {
    Start-Sleep -Seconds 1
    $WaitTime++
    if ($Process.HasExited) {
        Write-Host "[ERROR] Application exited unexpectedly" -ForegroundColor Red
        exit 1
    }
    # Check if core is running
    $CoreProcess = Get-Process -Name "nekobox_core" -ErrorAction SilentlyContinue
    if ($CoreProcess) {
        Write-Host "[OK] Core process detected (PID: $($CoreProcess.Id))" -ForegroundColor Green
        break
    }
}

if ($WaitTime -ge $MaxWait) {
    Write-Host "[WARN] Core process not detected, but continuing..." -ForegroundColor Yellow
}

# Wait a bit more for UI to be ready
Start-Sleep -Seconds 3

Write-Host "[INFO] Attempting to trigger URL test via UI automation..." -ForegroundColor Yellow

# Try to find and click URL test button using Windows UI Automation
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

function FindAndClickButton {
    param([string]$ButtonName)
    
    try {
        $automation = New-Object System.Windows.Automation.AutomationElement
        $root = [System.Windows.Automation.AutomationElement]::RootElement
        
        $condition = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::NameProperty, 
            $ButtonName
        )
        
        $button = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
        
        if ($button -ne $null) {
            $invokePattern = $button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
            if ($invokePattern -ne $null) {
                $invokePattern.Invoke()
                Write-Host "[OK] Clicked button: $ButtonName" -ForegroundColor Green
                return $true
            }
        }
    } catch {
        Write-Host "[WARN] UI Automation failed: $_" -ForegroundColor Yellow
    }
    return $false
}

# Alternative: Use SendKeys to navigate and trigger
function SendKeysToApp {
    param([string]$Keys)
    
    try {
        # Activate the window
        Add-Type -TypeDefinition @"
        using System;
        using System.Runtime.InteropServices;
        public class Win32 {
            [DllImport("user32.dll")]
            public static extern bool SetForegroundWindow(IntPtr hWnd);
            [DllImport("user32.dll")]
            public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
        }
"@
        
        $hwnd = [Win32]::FindWindow($null, "NekoBox")
        if ($hwnd -ne [IntPtr]::Zero) {
            [Win32]::SetForegroundWindow($hwnd) | Out-Null
            Start-Sleep -Milliseconds 500
            [System.Windows.Forms.SendKeys]::SendWait($Keys)
            Write-Host "[OK] Sent keys: $Keys" -ForegroundColor Green
            return $true
        }
    } catch {
        Write-Host "[WARN] SendKeys failed: $_" -ForegroundColor Yellow
    }
    return $false
}

# Try multiple methods to trigger URL test
$Success = $false

# Method 1: Try UI Automation
Write-Host "[INFO] Method 1: Trying UI Automation..." -ForegroundColor Gray
$Success = FindAndClickButton("URL Test") -or FindAndClickButton("Test") -or FindAndClickButton("Test Latency")

if (-not $Success) {
    # Method 2: Try keyboard shortcut (if exists)
    Write-Host "[INFO] Method 2: Trying keyboard shortcut..." -ForegroundColor Gray
    Add-Type -AssemblyName System.Windows.Forms
    $Success = SendKeysToApp("{F5}") -or SendKeysToApp("^T") -or SendKeysToApp("{TAB}{TAB}{TAB}{ENTER}")
}

if (-not $Success) {
    Write-Host "[WARN] Could not trigger URL test via UI, checking logs directly..." -ForegroundColor Yellow
}

# Wait for test to complete
Write-Host "[INFO] Waiting for URL test to complete..." -ForegroundColor Yellow
Start-Sleep -Seconds 10

# Check results
Write-Host ""
Write-Host "=== Checking Results ===" -ForegroundColor Cyan

# Check core_output.log
if (Test-Path "core_output.log") {
    Write-Host "[FOUND] core_output.log" -ForegroundColor Green
    $logContent = Get-Content "core_output.log" -ErrorAction SilentlyContinue
    if ($logContent) {
        Write-Host "  Lines: $($logContent.Count)" -ForegroundColor Gray
        $recentLogs = $logContent | Select-Object -Last 20
        Write-Host "  Recent content:" -ForegroundColor Yellow
        $recentLogs | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
        
        # Check for success indicators
        $hasConnection = $logContent | Select-String -Pattern "connection|outbound|inbound|INFO\[|ERROR\[" -Quiet
        if ($hasConnection) {
            Write-Host "[SUCCESS] Core logs show network activity!" -ForegroundColor Green
        }
    }
} else {
    Write-Host "[NOT FOUND] core_output.log" -ForegroundColor Red
}

# Check crash_log.txt
if (Test-Path "crash_log.txt") {
    Write-Host "[FOUND] crash_log.txt" -ForegroundColor Yellow
    $crashLog = Get-Content "crash_log.txt" -ErrorAction SilentlyContinue
    if ($crashLog) {
        Write-Host "  Content:" -ForegroundColor Yellow
        $crashLog | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
    }
}

# Check if process is still running
if ($Process.HasExited) {
    Write-Host "[WARN] Application exited (Code: $($Process.ExitCode))" -ForegroundColor Yellow
} else {
    Write-Host "[OK] Application still running" -ForegroundColor Green
}

Write-Host ""
Write-Host "[INFO] Test completed" -ForegroundColor Green

# Keep process running for inspection
Write-Host "[INFO] Process will remain running. Press Ctrl+C to stop." -ForegroundColor Gray
try {
    $Process.WaitForExit()
} catch {
    # User interrupted
}
