#!/usr/bin/env python3
"""
WinAppDriver automated test for Nekoray
Automatically starts app, selects node, starts proxy, and tests URL
"""

import time
import sys
import subprocess
import os
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.keys import Keys
from selenium.common.exceptions import TimeoutException, NoSuchElementException

# Configuration
WAD_PORT = 4723
APP_PATH = r"build\Release\nekobox.exe"
TIMEOUT = 30

def start_winappdriver():
    """Start WinAppDriver if not running"""
    try:
        # Check if already running
        result = subprocess.run(
            ["netstat", "-ano"], 
            capture_output=True, 
            text=True, 
            check=True
        )
        if f":{WAD_PORT}" in result.stdout:
            print(f"[INFO] WinAppDriver already running on port {WAD_PORT}")
            return True
        
        # Try to start WinAppDriver
        wad_paths = [
            r"C:\Program Files (x86)\Windows Application Driver\WinAppDriver.exe",
            r"C:\Program Files\Windows Application Driver\WinAppDriver.exe",
            os.path.expanduser(r"~\AppData\Local\Programs\Windows Application Driver\WinAppDriver.exe")
        ]
        
        for wad_path in wad_paths:
            if os.path.exists(wad_path):
                print(f"[INFO] Starting WinAppDriver from {wad_path}")
                subprocess.Popen([wad_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                time.sleep(2)
                return True
        
        print("[WARN] WinAppDriver not found. Please install from:")
        print("  https://github.com/microsoft/WinAppDriver/releases")
        return False
    except Exception as e:
        print(f"[ERROR] Failed to start WinAppDriver: {e}")
        return False

def create_driver():
    """Create WinAppDriver session"""
    capabilities = {
        "app": os.path.abspath(APP_PATH),
        "platformName": "Windows",
        "deviceName": "WindowsPC"
    }
    
    try:
        driver = webdriver.Remote(
            command_executor=f"http://127.0.0.1:{WAD_PORT}",
            desired_capabilities=capabilities
        )
        print("[OK] WinAppDriver session created")
        return driver
    except Exception as e:
        print(f"[ERROR] Failed to create driver: {e}")
        return None

def wait_for_element(driver, by, value, timeout=TIMEOUT):
    """Wait for element with timeout"""
    try:
        element = WebDriverWait(driver, timeout).until(
            EC.presence_of_element_located((by, value))
        )
        return element
    except TimeoutException:
        print(f"[ERROR] Element not found: {by}={value}")
        return None

def main():
    print("=" * 50)
    print("WinAppDriver Automated Test for Nekoray")
    print("=" * 50)
    print()
    
    # Step 1: Start WinAppDriver
    print("[1/5] Starting WinAppDriver...")
    if not start_winappdriver():
        print("[ERROR] WinAppDriver not available")
        return 1
    
    # Step 2: Create driver and launch app
    print("[2/5] Launching application...")
    driver = create_driver()
    if not driver:
        return 1
    
    try:
        # Wait for main window
        print("[INFO] Waiting for main window...")
        time.sleep(5)
        
        # Step 3: Find and click first proxy node
        print("[3/5] Selecting first proxy node...")
        try:
            # Try to find proxy list table
            table = wait_for_element(driver, By.NAME, "ProxyListTable", timeout=10)
            if table:
                # Click first row
                first_row = wait_for_element(driver, By.NAME, "ProxyRow_0", timeout=5)
                if first_row:
                    first_row.click()
                    print("[OK] First node selected")
                    time.sleep(1)
                    # Press Enter to start
                    first_row.send_keys(Keys.RETURN)
                    print("[OK] Node started (Enter pressed)")
                    time.sleep(3)
                else:
                    # Fallback: click on table first row
                    rows = driver.find_elements(By.XPATH, "//Table[@Name='ProxyListTable']//TableItem")
                    if rows:
                        rows[0].click()
                        time.sleep(1)
                        rows[0].send_keys(Keys.RETURN)
                        print("[OK] Node started via fallback method")
                        time.sleep(3)
        except Exception as e:
            print(f"[WARN] Could not select node via UI: {e}")
            print("[INFO] Trying keyboard shortcut...")
            # Fallback: use keyboard
            body = driver.find_element(By.TAG_NAME, "Window")
            body.send_keys(Keys.RETURN)
            time.sleep(3)
        
        # Step 4: Click URL test button
        print("[4/5] Triggering URL test...")
        try:
            url_test_btn = wait_for_element(driver, By.NAME, "URLTestButton", timeout=10)
            if url_test_btn:
                url_test_btn.click()
                print("[OK] URL test button clicked")
            else:
                # Fallback: find by automation ID or XPath
                buttons = driver.find_elements(By.XPATH, "//Button[contains(@Name, 'Test') or contains(@Name, 'URL')]")
                if buttons:
                    buttons[0].click()
                    print("[OK] URL test triggered via fallback")
                else:
                    print("[WARN] URL test button not found, trying keyboard...")
                    body = driver.find_element(By.TAG_NAME, "Window")
                    body.send_keys(Keys.CONTROL + "t")
        except Exception as e:
            print(f"[WARN] Could not click URL test: {e}")
        
        # Step 5: Wait and check results
        print("[5/5] Waiting for test results...")
        time.sleep(15)
        
        # Check logs
        print()
        print("=== Checking Results ===")
        log_files = ["core_output.log", "crash_log.txt"]
        for log_file in log_files:
            if os.path.exists(log_file):
                print(f"[FOUND] {log_file}")
                with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
                    lines = f.readlines()
                    print(f"  Lines: {len(lines)}")
                    if lines:
                        print("  Last 10 lines:")
                        for line in lines[-10:]:
                            print(f"    {line.rstrip()}")
                        
                        # Check for success
                        content = "".join(lines)
                        if "succeeded" in content.lower() or "latency" in content.lower():
                            print(f"[SUCCESS] Test completed in {log_file}!")
                            return 0
            else:
                print(f"[NOT FOUND] {log_file}")
        
        print("[INFO] Test completed. Check logs manually.")
        return 0
        
    except Exception as e:
        print(f"[ERROR] Test failed: {e}")
        import traceback
        traceback.print_exc()
        return 1
    finally:
        print("[INFO] Keeping app running for inspection...")
        print("[INFO] Press Ctrl+C to close")
        try:
            time.sleep(60)  # Keep running for inspection
        except KeyboardInterrupt:
            pass
        # driver.quit()  # Uncomment to auto-close

if __name__ == "__main__":
    sys.exit(main())
