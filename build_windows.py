#!/usr/bin/env python3
"""Flag-free Windows build helper (Python only).

- Always builds Release with Qt6.
- Always ensures deps are downloaded/built and cached under libs\\deps\\built.
- Always downloads public resources and prebuilt nekobox_core after build.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

CONFIG = "Release"
DEPS_BUILD_TIMEOUT = 900
DOWNLOAD_TIMEOUT = 300
QT_REL_PATH = Path("qt_lib/qt650")
DEPS_BUILD_DIR_REL = Path("libs/deps")
DEPS_ROOT_REL = DEPS_BUILD_DIR_REL / "built"


# ---------- logging ----------

def info(msg: str) -> None:
    print(f"[INFO] {msg}")


def error(msg: str) -> None:
    print(f"[ERROR] {msg}", file=sys.stderr)


# ---------- utils ----------

def run_command(cmd: list[str], *, env: dict | None = None, timeout: int | None = None, capture: bool = False) -> subprocess.CompletedProcess:
    info("Running: " + " ".join(cmd))
    try:
        return subprocess.run(
            cmd,
            check=True,
            env=env,
            timeout=timeout,
            text=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.STDOUT if capture else None,
        )
    except subprocess.CalledProcessError as exc:
        if capture and exc.stdout:
            error(exc.stdout)
        error(f"Command failed (exit {exc.returncode}): {' '.join(cmd)}")
        raise


def require_path(path: Path, message: str) -> Path:
    if not path.exists():
        error(message)
        sys.exit(1)
    return path


def any_path(paths: list[Path]) -> bool:
    return any(p.exists() for p in paths)


def prepare_x64_env(env: dict) -> dict:
    new_env = env.copy()
    new_env.setdefault("VSCMD_ARG_TGT_ARCH", "x64")
    new_env.setdefault("VSCMD_ARG_HOST_ARCH", "x64")
    new_env.setdefault("PreferredToolArchitecture", "x64")
    return new_env


# ---------- VS / generator ----------

def find_vs_dev_cmd() -> Path | None:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        return None
    result = run_command(
        [str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild", "-property", "installationPath"],
        capture=True,
    )
    base = Path(result.stdout.strip()) if result.stdout else None
    if not base:
        return None
    dev_cmd = base / "Common7" / "Tools" / "VsDevCmd.bat"
    return dev_cmd if dev_cmd.exists() else None


def import_vs_env(vs_dev_cmd: Path) -> dict:
    result = run_command(["cmd", "/c", f"\"{vs_dev_cmd}\" -no_logo && set"], capture=True)
    env = os.environ.copy()
    for line in result.stdout.splitlines():
        if "=" in line:
            key, val = line.split("=", 1)
            env[key] = val
    return env


def detect_generator() -> tuple[bool, str, dict]:
    env = os.environ.copy()
    ninja_path = shutil.which("ninja")
    if not ninja_path:
        return False, "", env
    vs_dev_cmd = find_vs_dev_cmd()
    if not vs_dev_cmd:
        info("Ninja detected but VS dev environment unavailable; falling back to default generator.")
        return False, "", env
    info("Using generator: Ninja Multi-Config")
    env = import_vs_env(vs_dev_cmd)
    return True, "Ninja Multi-Config", env


# ---------- dependency helpers ----------

def call_ps_script(script: Path, *, env: dict | None = None, timeout: int | None = None) -> None:
    require_path(script, f"Script not found: {script}")
    run_command(["powershell", "-ExecutionPolicy", "Bypass", "-File", str(script)], env=env, timeout=timeout)


def ensure_third_party(repo_root: Path, deps_root: Path, build_dir: Path, env: dict) -> None:
    script = repo_root / "libs" / "build_deps_windows.ps1"
    call_ps_script(script, env=env, timeout=DEPS_BUILD_TIMEOUT)
    yaml_configs = [
        deps_root / "lib/cmake/yaml-cpp/yaml-cpp-config.cmake",
        deps_root / "lib/cmake/yaml-cpp/yaml-cppConfig.cmake",
        deps_root / "cmake/yaml-cpp/yaml-cpp-config.cmake",
        deps_root / "cmake/yaml-cpp/yaml-cppConfig.cmake",
        deps_root / "share/cmake/yaml-cpp/yaml-cpp-config.cmake",
        deps_root / "share/cmake/yaml-cpp/yaml-cppConfig.cmake",
        deps_root / "share/yaml-cpp/yaml-cpp-config.cmake",
        deps_root / "share/yaml-cpp/yaml-cppConfig.cmake",
    ]
    zxing_configs = [
        deps_root / "lib/cmake/ZXing/ZXingConfig.cmake",
        deps_root / "lib/cmake/ZXing/ZXing-config.cmake",
        deps_root / "cmake/ZXing/ZXingConfig.cmake",
        deps_root / "cmake/ZXing/ZXing-config.cmake",
        deps_root / "share/cmake/ZXing/ZXingConfig.cmake",
        deps_root / "share/cmake/ZXing/ZXing-config.cmake",
        deps_root / "share/cmake/zxing-cpp/zxing-cpp-config.cmake",
        deps_root / "share/zxing-cpp/zxing-cpp-config.cmake",
        deps_root / "share/ZXing/ZXingConfig.cmake",
    ]
    if not any_path(yaml_configs):
        error(f"yaml-cpp not found in '{deps_root}' after build")
        sys.exit(1)
    if not any_path(zxing_configs):
        error(f"ZXing not found in '{deps_root}' after build")
        sys.exit(1)


def ensure_protobuf(repo_root: Path, deps_root: Path, env: dict) -> None:
    if protobuf_installed(deps_root):
        return
    script = repo_root / "libs" / "build_protobuf_windows.ps1"
    call_ps_script(script, env=prepare_x64_env(env), timeout=DEPS_BUILD_TIMEOUT)
    if not protobuf_installed(deps_root):
        error(f"Protobuf not found in '{deps_root}' after build")
        sys.exit(1)


def protobuf_installed(deps_root: Path) -> bool:
    protoc = [
        deps_root / "bin/protoc.exe",
        deps_root / "tools/protobuf/protoc.exe",
    ]
    libs = [
        deps_root / "lib/libprotobuf.lib",
        deps_root / "lib/protobuf.lib",
    ]
    configs = [
        deps_root / "cmake/protobuf-config.cmake",
        deps_root / "cmake/protobuf/protobuf-config.cmake",
        deps_root / "cmake/protobuf/protobufConfig.cmake",
        deps_root / "share/protobuf/protobuf-config.cmake",
        deps_root / "share/protobuf/protobufConfig.cmake",
    ]
    return any_path(protoc) and any_path(libs) and any_path(configs)


# ---------- build steps ----------

def configure_cmake(repo_root: Path, build_dir: Path, qt_root: Path, deps_root: Path, generator: str, env: dict) -> None:
    args = [
        "cmake",
        "-S",
        str(repo_root),
        "-B",
        str(build_dir),
        "-DQT_VERSION_MAJOR=6",
        f"-DNKR_LIBS={deps_root}",
        "-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false",
        f"-DCMAKE_PREFIX_PATH={qt_root}",
    ]
    if generator:
        args = ["cmake", "-G", generator] + args[1:]
    run_command(args, env=env)


def build_project(build_dir: Path, env: dict, use_ninja: bool) -> None:
    info(f"Building (Config={CONFIG})...")
    args = ["cmake", "--build", str(build_dir), "--config", CONFIG]
    if not use_ninja:
        args.extend(["--", "/m:1", "/p:TrackFileAccess=false", "/p:EnableMinimalRebuild=false"])
    run_command(args, env=env)


def release_dir_for(build_dir: Path, use_ninja: bool) -> Path:
    return build_dir if use_ninja else build_dir / CONFIG


def copy_executable(release_dir: Path, repo_root: Path) -> None:
    built = require_path(release_dir / "nekobox.exe", "Built exe not found after build")
    target = repo_root / "nekobox.exe"
    info(f"Copying to repo root: {target}")
    shutil.copy2(built, target)


def download_resources(repo_root: Path, release_dir: Path) -> None:
    script = repo_root / "libs" / "download_resources.ps1"
    if not script.exists():
        info("download_resources.ps1 missing; skipping public resources.")
        return
    info("Downloading public resources...")
    try:
        run_command(
            ["powershell", "-ExecutionPolicy", "Bypass", "-File", str(script), "-DestDir", str(release_dir)],
            timeout=DOWNLOAD_TIMEOUT,
        )
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        info("Public resource download failed; continuing.")


def download_core(repo_root: Path, release_dir: Path) -> None:
    script = repo_root / "libs" / "download_core.ps1"
    if not script.exists():
        info("download_core.ps1 missing; skipping nekobox_core.")
        return
    info("Downloading nekobox_core...")
    try:
        run_command(
            ["powershell", "-ExecutionPolicy", "Bypass", "-File", str(script), "-DestDir", str(release_dir)],
            timeout=DOWNLOAD_TIMEOUT,
        )
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        info("nekobox_core download failed; continuing.")


# ---------- main ----------

def main() -> None:
    repo_root = Path(__file__).resolve().parent
    qt_root = require_path(repo_root / QT_REL_PATH, f"QtRoot missing: {QT_REL_PATH}")

    run_command(["cmake", "--version"])

    use_ninja, generator, env = detect_generator()
    build_dir = repo_root / ("build_ninja" if use_ninja else "build")
    deps_root = repo_root / DEPS_ROOT_REL
    deps_root.mkdir(parents=True, exist_ok=True)

    ensure_third_party(repo_root, deps_root, repo_root / DEPS_BUILD_DIR_REL, env)
    ensure_protobuf(repo_root, deps_root, env)

    configure_cmake(repo_root, build_dir, qt_root, deps_root, generator, env)
    build_project(build_dir, env, use_ninja)

    release_dir = release_dir_for(build_dir, use_ninja)
    copy_executable(release_dir, repo_root)
    download_resources(repo_root, release_dir)
    download_core(repo_root, release_dir)

    info("Build completed successfully.")


if __name__ == "__main__":
    main()
