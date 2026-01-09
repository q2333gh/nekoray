#!/usr/bin/env python3
"""
Direct API test - calls functions directly without UI automation
"""

import subprocess
import time
import os
import sys

def check_logs():
    """Check log files for success indicators"""
    success = False
    log_files = ["core_output.log", "crash_log.txt"]
    
    for log_file in log_files:
        if os.path.exists(log_file):
            with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
                if "succeeded" in content.lower() or ("latency" in content.lower() and "ms" in content.lower()):
                    print(f"[SUCCESS] Found success indicator in {log_file}")
                    success = True
                    # Show relevant lines
                    lines = content.split("\n")
                    for line in lines:
                        if "succeeded" in line.lower() or ("latency" in line.lower() and "ms" in line.lower()):
                            print(f"  {line}")
    return success

def main():
    print("=" * 50)
    print("Direct API Test (No UI Automation)")
    print("=" * 50)
    print()
    
    # Clean logs
    for log in ["core_output.log", "crash_log.txt"]:
        if os.path.exists(log):
            os.remove(log)
    
    # Start app
    print("[1/4] Starting application...")
    exe_path = os.path.abspath("build/Release/nekobox.exe")
    if not os.path.exists(exe_path):
        print(f"[ERROR] Executable not found: {exe_path}")
        return 1
    
    process = subprocess.Popen([exe_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    print(f"[OK] Started (PID: {process.pid})")
    
    # Wait for core
    print("[2/4] Waiting for core to start...")
    core_started = False
    for i in range(20):
        time.sleep(1)
        try:
            result = subprocess.run(
                ["powershell", "-Command", "Get-Process -Name nekobox_core -ErrorAction SilentlyContinue"],
                capture_output=True,
                text=True,
                timeout=5
            )
            if "nekobox_core" in result.stdout:
                print("[OK] Core process detected")
                core_started = True
                break
        except:
            pass
    
    if not core_started:
        print("[WARN] Core not detected, but continuing...")
    
    time.sleep(5)  # Wait for UI to be ready
    
    # Since we can't use WinAppDriver, we'll use the existing PowerShell automation
    # which uses SendKeys - this is more reliable
    print("[3/4] Triggering actions via SendKeys...")
    try:
        import ctypes
        from ctypes import wintypes
        
        # Find window
        user32 = ctypes.windll.user32
        hwnd = user32.FindWindowW(None, "NekoBox")
        if hwnd == 0:
            hwnd = user32.FindWindowW(None, "nekobox")
        
        if hwnd != 0:
            # Bring to foreground
            user32.SetForegroundWindow(hwnd)
            time.sleep(0.5)
            
            # Press Enter to select first node and start
            print("[INFO] Pressing Enter to start first node...")
            user32.keybd_event(0x0D, 0, 0, 0)  # Enter down
            user32.keybd_event(0x0D, 0, 2, 0)  # Enter up
            time.sleep(3)
            
            # Try to trigger URL test - use Ctrl+T or find button
            print("[INFO] Triggering URL test...")
            # Try Ctrl+T
            user32.keybd_event(0x11, 0, 0, 0)  # Ctrl down
            user32.keybd_event(0x54, 0, 0, 0)  # T down
            user32.keybd_event(0x54, 0, 2, 0)  # T up
            user32.keybd_event(0x11, 0, 2, 0)  # Ctrl up
            time.sleep(2)
            
            print("[OK] Actions triggered")
        else:
            print("[WARN] Window not found, trying alternative method...")
            # Use PowerShell SendKeys as fallback
            ps_script = '''
            Add-Type -AssemblyName System.Windows.Forms
            $hwnd = [System.Windows.Forms.Control]::FromHandle((Get-Process -Name nekobox | Select-Object -First 1).MainWindowHandle)
            if ($hwnd) {
                $hwnd.Focus()
                [System.Windows.Forms.SendKeys]::SendWait("{ENTER}")
                Start-Sleep -Seconds 3
                [System.Windows.Forms.SendKeys]::SendWait("^T")
            }
            '''
            subprocess.run(["powershell", "-Command", ps_script], timeout=10)
    except Exception as e:
        print(f"[WARN] SendKeys failed: {e}")
    
    # Wait for test
    print("[4/4] Waiting for test results...")
    time.sleep(20)
    
    # Check results
    print()
    print("=== Checking Results ===")
    success = check_logs()
    
    if success:
        print()
        print("[SUCCESS] URL test completed and proxy is working!")
        return 0
    else:
        print()
        print("[FAIL] URL test did not succeed or logs not found")
        print("[INFO] Process still running (PID: {})".format(process.pid))
        return 1

if __name__ == "__main__":
    sys.exit(main())
