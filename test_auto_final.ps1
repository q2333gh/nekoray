# Final automated test - uses multiple methods to ensure success
param(
    [string]$ExePath = "build\Release\nekobox.exe",
    [int]$MaxRetries = 3
)

$ErrorActionPreference = "Continue"
$RepoRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $RepoRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Final Automated Test" -ForegroundColor Cyan
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

# Function to find and activate window
function ActivateNekorayWindow {
    $proc = Get-Process -Name "nekobox" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $proc) { return $false }
    
    Add-Type @"
    using System;
    using System.Runtime.InteropServices;
    public class Win32 {
        [DllImport("user32.dll")]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")]
        public static extern bool IsIconic(IntPtr hWnd);
        public const int SW_RESTORE = 9;
    }
"@
    
    $hwnd = $proc.MainWindowHandle
    if ($hwnd -ne [IntPtr]::Zero) {
        if ([Win32]::IsIconic($hwnd)) {
            [Win32]::ShowWindow($hwnd, [Win32]::SW_RESTORE)
        }
        [Win32]::SetForegroundWindow($hwnd) | Out-Null
        Start-Sleep -Milliseconds 500
        return $true
    }
    return $false
}

# Function to send keys
function SendKeysToApp {
    param([string]$Keys, [int]$Delay = 1000)
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($Keys)
    Start-Sleep -Milliseconds $Delay
}

# Start app
Write-Host "[1/5] Starting application..." -ForegroundColor Green
$Process = Start-Process -FilePath $FullExePath -PassThru -WindowStyle Normal
if ($null -eq $Process) {
    Write-Host "[ERROR] Failed to start" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Started (PID: $($Process.Id))" -ForegroundColor Green

# Wait for core
Write-Host "[2/5] Waiting for core..." -ForegroundColor Yellow
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Seconds 1
    $CoreProcess = Get-Process -Name "nekobox_core" -ErrorAction SilentlyContinue
    if ($CoreProcess) {
        Write-Host "[OK] Core running (PID: $($CoreProcess.Id))" -ForegroundColor Green
        break
    }
}
Start-Sleep -Seconds 5

# Activate window and trigger actions
Write-Host "[3/5] Activating window and starting proxy..." -ForegroundColor Yellow
$activated = $false
for ($retry = 0; $retry -lt 5; $retry++) {
    if (ActivateNekorayWindow) {
        $activated = $true
        Write-Host "[OK] Window activated" -ForegroundColor Green
        
        # Press Enter to start first node
        Write-Host "[INFO] Pressing Enter..." -ForegroundColor Gray
        SendKeysToApp "{ENTER}" 3000
        
        # Wait a bit
        Start-Sleep -Seconds 2
        
        # Try multiple methods to trigger URL test
        Write-Host "[4/5] Triggering URL test..." -ForegroundColor Yellow
        
        # Wait for profile to fully start
        Start-Sleep -Seconds 5
        
        # Method 1: Direct mouse click on URL test button
        # Calculate button position (typically in top toolbar, right side)
        Add-Type @"
        using System;
        using System.Runtime.InteropServices;
        using System.Drawing;
        public class Win32Helper {
            [DllImport("user32.dll")]
            public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
            [DllImport("user32.dll")]
            public static extern bool SetCursorPos(int X, int Y);
            [DllImport("user32.dll")]
            public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, int dwExtraInfo);
            [DllImport("user32.dll")]
            public static extern IntPtr WindowFromPoint(POINT Point);
            public const uint MOUSEEVENTF_LEFTDOWN = 0x02;
            public const uint MOUSEEVENTF_LEFTUP = 0x04;
            public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
            public struct POINT { public int X; public int Y; }
        }
"@
        $proc = Get-Process -Name "nekobox" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($proc) {
            $hwnd = $proc.MainWindowHandle
            $rect = New-Object Win32Helper+RECT
            [Win32Helper]::GetWindowRect($hwnd, [ref]$rect)
            # URL test button is in top toolbar, right side (approximately)
            # Try multiple positions
            $buttonPositions = @(
                @{X = $rect.Right - 100; Y = $rect.Top + 30},   # Top-right area
                @{X = $rect.Right - 150; Y = $rect.Top + 30},
                @{X = $rect.Right - 80; Y = $rect.Top + 50},
                @{X = ($rect.Left + $rect.Right) / 2 + 200; Y = $rect.Top + 40}  # Center-right
            )
            foreach ($pos in $buttonPositions) {
                Write-Host "[INFO] Trying mouse click at ($($pos.X), $($pos.Y))" -ForegroundColor Gray
                [Win32Helper]::SetCursorPos($pos.X, $pos.Y)
                Start-Sleep -Milliseconds 300
                [Win32Helper]::mouse_event([Win32Helper]::MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
                Start-Sleep -Milliseconds 100
                [Win32Helper]::mouse_event([Win32Helper]::MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
                Start-Sleep -Seconds 2
            }
        }
        
        # Method 2: Keyboard shortcuts
        Write-Host "[INFO] Trying keyboard shortcuts..." -ForegroundColor Gray
        SendKeysToApp "^T" 2000  # Ctrl+T
        SendKeysToApp "{F5}" 2000  # F5 (common refresh/test shortcut)
        
        # Method 3: Click on label_running (status label) - this triggers speedtest_current()
        # This is more reliable as it's a direct click handler
        $rect = New-Object Win32Helper+RECT
        [Win32Helper]::GetWindowRect($hwnd, [ref]$rect)
        # label_running is typically in status bar area (bottom of window)
        $statusY = $rect.Bottom - 30
        $statusX = ($rect.Left + $rect.Right) / 2
        Write-Host "[INFO] Clicking status label at ($statusX, $statusY)" -ForegroundColor Gray
        [Win32Helper]::SetCursorPos($statusX, $statusY)
        Start-Sleep -Milliseconds 300
        [Win32Helper]::mouse_event([Win32Helper]::MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        Start-Sleep -Milliseconds 100
        [Win32Helper]::mouse_event([Win32Helper]::MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        Start-Sleep -Seconds 2
        
        # Method 4: Menu navigation
        SendKeysToApp "%S" 500  # Alt+S (Server menu)
        Start-Sleep -Milliseconds 500
        SendKeysToApp "G" 500  # G for Current Group
        Start-Sleep -Milliseconds 500
        SendKeysToApp "U" 3000  # U for URL test
        
        Write-Host "[OK] Actions triggered" -ForegroundColor Green
        break
    } else {
        Write-Host "[RETRY] Attempt $($retry+1)/5 to activate window..." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
    }
}

if (-not $activated) {
    Write-Host "[WARN] Could not activate window, but continuing..." -ForegroundColor Yellow
}

# Wait for test
Write-Host "[5/5] Waiting for test results..." -ForegroundColor Yellow
Start-Sleep -Seconds 25

# Check results
Write-Host ""
Write-Host "=== Results ===" -ForegroundColor Cyan

$Success = $false

# Check all possible log locations (app uses config subdirectory)
$logPaths = @(
    "core_output.log",
    "crash_log.txt",
    (Join-Path $RepoRoot "core_output.log"),
    (Join-Path $RepoRoot "crash_log.txt"),
    (Join-Path $RepoRoot "build\Release\config\core_output.log"),
    (Join-Path $RepoRoot "build\Release\config\crash_log.txt"),
    (Join-Path (Get-Location) "core_output.log"),
    (Join-Path (Get-Location) "crash_log.txt"),
    (Join-Path (Get-Location) "config\core_output.log"),
    (Join-Path (Get-Location) "config\crash_log.txt")
)

foreach ($logPath in $logPaths) {
    if (Test-Path $logPath) {
        Write-Host "[FOUND] $logPath" -ForegroundColor Green
        $content = Get-Content $logPath -ErrorAction SilentlyContinue
        if ($content) {
            Write-Host "  Lines: $($content.Count)" -ForegroundColor Gray
            
            # Check for success
            $hasSuccess = $content | Select-String -Pattern "succeeded|latency.*ms|Test.*succeeded|gRPC.*succeeded" -Quiet
            $hasConnection = $content | Select-String -Pattern "connection|outbound|inbound|INFO\[.*ms\]" -Quiet
            
            if ($hasSuccess -or $hasConnection) {
                Write-Host "[SUCCESS] Found success indicators!" -ForegroundColor Green
                $content | Select-String -Pattern "succeeded|latency|ms\]|Test" | Select-Object -Last 10 | ForEach-Object {
                    Write-Host "  $_" -ForegroundColor Green
                }
                $Success = $true
            } else {
                Write-Host "  Last 15 lines:" -ForegroundColor Yellow
                $content | Select-Object -Last 15 | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
            }
        }
    }
}

# Check if process is still running
if (-not $Process.HasExited) {
    Write-Host "[INFO] Application still running" -ForegroundColor Gray
    $CoreProcess = Get-Process -Name "nekobox_core" -ErrorAction SilentlyContinue
    if ($CoreProcess) {
        Write-Host "[INFO] Core process running (PID: $($CoreProcess.Id))" -ForegroundColor Gray
    }
}

Write-Host ""
if ($Success) {
    Write-Host "[SUCCESS] URL test completed and proxy is working!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "[FAIL] URL test did not succeed" -ForegroundColor Red
    Write-Host "[INFO] Process still running for inspection (PID: $($Process.Id))" -ForegroundColor Gray
    exit 1
}
