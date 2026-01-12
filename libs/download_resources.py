#!/usr/bin/env python3
"""Download geosite, geodb, and other public resources for Windows build."""

from __future__ import annotations

import sys
from pathlib import Path
import urllib.request
import shutil

REPO_ROOT = Path(__file__).resolve().parent.parent

GEODATA_URLS = {
    "geoip.dat": "https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/geoip.dat",
    "geosite.dat": "https://github.com/v2fly/domain-list-community/releases/latest/download/dlc.dat",
    "geoip.db": "https://github.com/SagerNet/sing-geoip/releases/latest/download/geoip.db",
    "geosite.db": "https://github.com/SagerNet/sing-geosite/releases/latest/download/geosite.db",
}


def info(msg: str) -> None:
    print(f"[INFO] {msg}")


def error(msg: str) -> None:
    print(f"[ERROR] {msg}", file=sys.stderr)


def download_file(url: str, dest_path: Path) -> None:
    """Download a file from URL to destination path."""
    try:
        with urllib.request.urlopen(url, timeout=300) as response:
            with open(dest_path, "wb") as f:
                shutil.copyfileobj(response, f)
        file_size = dest_path.stat().st_size
        info(f"Successfully downloaded {dest_path.name} ({file_size} bytes)")
    except Exception as e:
        error(f"Failed to download {dest_path.name} from {url}")
        error(f"Error: {e}")
        sys.exit(1)


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description="Download public resources")
    parser.add_argument(
        "--dest-dir",
        type=str,
        default="",
        help="Destination directory (default: build/Release)",
    )
    args = parser.parse_args()

    if args.dest_dir:
        dest_dir = Path(args.dest_dir).resolve()
    else:
        dest_dir = REPO_ROOT / "build" / "Release"

    dest_dir.mkdir(parents=True, exist_ok=True)
    info(f"Downloading public resources to: {dest_dir}")

    # Download geodata files
    for filename, url in GEODATA_URLS.items():
        dest_path = dest_dir / filename

        # Check if file already exists and has content
        if dest_path.exists():
            file_size = dest_path.stat().st_size
            if file_size > 0:
                info(f"{filename} already exists ({file_size} bytes), skipping download")
                continue
            else:
                info(f"{filename} exists but is empty, re-downloading...")

        info(f"Downloading {filename}...")
        download_file(url, dest_path)

    # Copy res/public files if they exist
    public_res_dir = REPO_ROOT / "res" / "public"
    if public_res_dir.exists():
        info("Copying res/public files...")
        for file in public_res_dir.iterdir():
            if file.is_file():
                shutil.copy2(file, dest_dir / file.name)
                info(f"Copied {file.name}")
    else:
        info("res/public directory not found, skipping")

    info("Public resources download completed.")


if __name__ == "__main__":
    main()
