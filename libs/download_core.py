#!/usr/bin/env python3
"""Download prebuilt nekobox_core from GitHub Releases."""

from __future__ import annotations

import json
import shutil
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

REPO_OWNER = "MatsuriDayo"
REPO_NAME = "nekoray"
CORE_EXE_NAME = "nekobox_core.exe"


def info(msg: str) -> None:
    print(f"[INFO] {msg}")


def error(msg: str) -> None:
    print(f"[ERROR] {msg}", file=sys.stderr)


def warn(msg: str) -> None:
    print(f"[WARN] {msg}", file=sys.stderr)


def download_file(url: str, dest_path: Path) -> None:
    """Download a file from URL to destination path."""
    try:
        with urllib.request.urlopen(url, timeout=300) as response:
            with open(dest_path, "wb") as f:
                shutil.copyfileobj(response, f)
    except Exception as e:
        raise RuntimeError(f"Failed to download from {url}: {e}")


def fetch_latest_release() -> tuple[str, dict]:
    """Fetch latest release info from GitHub API."""
    api_url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases/latest"
    try:
        with urllib.request.urlopen(api_url, timeout=30) as response:
            release_info = json.loads(response.read())
            return release_info["tag_name"], release_info
    except Exception:
        # Fallback: try releases list
        api_url = f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/releases"
        with urllib.request.urlopen(api_url, timeout=30) as response:
            releases = json.loads(response.read())
            if releases:
                return releases[0]["tag_name"], releases[0]
        raise RuntimeError("No releases found")


def find_windows_asset(release_info: dict) -> dict:
    """Find Windows release zip asset."""
    for asset in release_info.get("assets", []):
        name = asset.get("name", "")
        if "windows" in name.lower() and name.endswith(".zip"):
            return asset
    raise RuntimeError("No Windows release zip found in release")


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description="Download nekobox_core")
    parser.add_argument(
        "--dest-dir",
        type=str,
        default="",
        help="Destination directory (default: build/Release)",
    )
    parser.add_argument(
        "--version",
        type=str,
        default="latest",
        help="Version to download (default: latest)",
    )
    args = parser.parse_args()

    if args.dest_dir:
        dest_dir = Path(args.dest_dir).resolve()
    else:
        dest_dir = REPO_ROOT / "build" / "Release"

    dest_dir.mkdir(parents=True, exist_ok=True)

    info("Downloading prebuilt nekobox_core...")
    info(f"Destination: {dest_dir}")
    info(f"Version: {args.version}")

    temp_dir = Path(tempfile.gettempdir()) / "nekoray_core_download"
    if temp_dir.exists():
        shutil.rmtree(temp_dir)
    temp_dir.mkdir(parents=True)

    try:
        if args.version == "latest":
            # Get latest release
            info("Fetching latest release info...")
            version, release_info = fetch_latest_release()
            info(f"Latest release: {version}")

            # Find Windows release asset
            asset = find_windows_asset(release_info)
            download_url = asset["browser_download_url"]
            zip_filename = asset["name"]
        else:
            # Specific version
            version = args.version
            zip_filename = f"nekoray-{version}-windows64.zip"
            download_url = (
                f"https://github.com/{REPO_OWNER}/{REPO_NAME}/releases/"
                f"download/{version}/{zip_filename}"
            )

        zip_path = temp_dir / zip_filename

        info(f"Downloading: {download_url}")
        info(f"Saving to: {zip_path}")

        # Download zip file
        download_file(download_url, zip_path)

        info("Extracting nekobox_core.exe from zip...")

        # Extract zip
        extract_dir = temp_dir / "extracted"
        with zipfile.ZipFile(zip_path, "r") as zip_ref:
            zip_ref.extractall(extract_dir)

        # Find nekobox_core.exe in extracted files
        core_exe_path = None
        for path in extract_dir.rglob(CORE_EXE_NAME):
            core_exe_path = path
            break

        if not core_exe_path:
            raise RuntimeError("nekobox_core.exe not found in release zip")

        # Copy to destination
        dest_exe_path = dest_dir / CORE_EXE_NAME
        shutil.copy2(core_exe_path, dest_exe_path)

        info(f"nekobox_core.exe downloaded successfully: {dest_exe_path}")

    except Exception as e:
        error(f"Failed to download nekobox_core: {e}")
        warn(f"You can manually download from: https://github.com/{REPO_OWNER}/{REPO_NAME}/releases")
        warn(f"Extract nekobox_core.exe and place it in: {dest_dir}")
        sys.exit(1)
    finally:
        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)

    info("Core download completed.")


if __name__ == "__main__":
    main()
