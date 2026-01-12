#!/usr/bin/env python3
"""Simple helper that wipes the build tree and reruns `build_windows.py`."""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD_DIRS = ["build", "build_ninja"]
BUILD_ARTIFACTS = ["nekobox.exe"]


def remove_path(path: Path, *, is_dir: bool) -> None:
    if not path.exists():
        return
    if is_dir:
        shutil.rmtree(path)
        print(f"Removed directory {path}")
    else:
        path.unlink()
        print(f"Removed file {path}")


def clean_build_tree(clean_deps: bool) -> None:
    for entry in BUILD_DIRS:
        remove_path(ROOT / entry, is_dir=True)
    for entry in BUILD_ARTIFACTS:
        remove_path(ROOT / entry, is_dir=False)

    release_core = ROOT / "build" / "Release" / "nekobox_core.exe"
    if release_core.exists():
        remove_path(release_core, is_dir=False)

    if clean_deps:
        deps_path = ROOT / "libs" / "deps" / "built"
        if deps_path.exists():
            remove_path(deps_path, is_dir=True)


def run_build_script(passthru: list[str]) -> None:
    cmd = [sys.executable, str(ROOT / "build_windows.py")] + passthru
    print(f"Running build command: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="Clean build driver for Nekoray.")
    parser.add_argument(
        "--clean-deps",
        action="store_true",
        help="Drop cached third-party builds in libs/deps/built before building.",
    )
    parser.add_argument(
        "build_args",
        nargs=argparse.REMAINDER,
        help="Extra arguments passed through to build_windows.py.",
    )

    args = parser.parse_args()
    clean_build_tree(args.clean_deps)
    run_build_script(args.build_args)


if __name__ == "__main__":
    main()
