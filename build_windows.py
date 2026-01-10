#!/usr/bin/env python3
"""Lightweight procedural replacement for build_windows.ps1."""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def info(msg: str) -> None:
    print(f"[INFO] {msg}")


def error(msg: str) -> None:
    print(f"[ERROR] {msg}", file=sys.stderr)


def run_command(
    cmd,
    *,
    timeout: int | None = None,
    capture: bool = False,
    env: dict | None = None,
) -> subprocess.CompletedProcess:
    info(f"Running: {' '.join(str(part) for part in cmd)}")
    try:
        return subprocess.run(
            cmd,
            check=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.STDOUT if capture else None,
            text=True,
            timeout=timeout,
            env=env,
        )
    except subprocess.TimeoutExpired as exc:
        error(f"Command timed out after {timeout}s: {' '.join(str(part) for part in cmd)}")
        raise
    except subprocess.CalledProcessError as exc:
        if capture and exc.stdout:
            error(exc.stdout)
        error(f"Command failed with exit code {exc.returncode}: {' '.join(str(part) for part in cmd)}")
        raise


def kill_process_by_name(name: str) -> None:
    cmd = ["tasklist", "/FI", f"IMAGENAME eq {name}.exe", "/NH"]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    if f"{name}.exe" not in result.stdout:
        return
    info(f"Terminating running process: {name}.exe")
    subprocess.run(["taskkill", "/IM", f"{name}.exe", "/F"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def require_path(path: Path, message: str) -> Path:
    if not path.exists():
        error(message)
        sys.exit(1)
    return path


def prepare_x64_env(base_env: dict) -> dict:
    env = base_env.copy()
    env["VSCMD_ARG_TGT_ARCH"] = "x64"
    env["VSCMD_ARG_HOST_ARCH"] = "x64"
    env["PreferredToolArchitecture"] = "x64"
    return env


def detect_generator() -> tuple[bool, str, dict]:
    env = os.environ.copy()
    ninja_path = shutil.which("ninja")
    if not ninja_path:
        return False, "", env

    vs_dev_cmd = find_vs_dev_cmd()
    if not vs_dev_cmd:
        info("Ninja detected but Visual Studio dev environment unavailable; falling back to default generator.")
        return False, "", env

    info("Using generator: Ninja Multi-Config")
    env = import_vs_env(vs_dev_cmd)
    return True, "Ninja Multi-Config", env


def determine_deps_root(args_deps: str, repo_root: Path) -> Path:
    default = repo_root / "libs" / "deps" / "built"
    package = repo_root / "libs" / "deps" / "package"
    if args_deps:
        return Path(args_deps)
    return package if package.exists() else default


def ensure_dependencies(
    deps_root: Path,
    default_root: Path,
    repo_root: Path,
    timeout: int,
    env: dict,
) -> None:
    yaml_configs = [
        deps_root / "lib" / "cmake" / "yaml-cpp" / "yaml-cpp-config.cmake",
        deps_root / "lib" / "cmake" / "yaml-cpp" / "yaml-cppConfig.cmake",
        deps_root / "cmake" / "yaml-cpp" / "yaml-cpp-config.cmake",
        deps_root / "cmake" / "yaml-cpp" / "yaml-cppConfig.cmake",
        deps_root / "share" / "cmake" / "yaml-cpp" / "yaml-cpp-config.cmake",
        deps_root / "share" / "cmake" / "yaml-cpp" / "yaml-cppConfig.cmake",
        deps_root / "share" / "yaml-cpp" / "yaml-cpp-config.cmake",
        deps_root / "share" / "yaml-cpp" / "yaml-cppConfig.cmake",
    ]
    zxing_configs = [
        deps_root / "lib" / "cmake" / "ZXing" / "ZXingConfig.cmake",
        deps_root / "lib" / "cmake" / "ZXing" / "ZXing-config.cmake",
        deps_root / "cmake" / "ZXing" / "ZXingConfig.cmake",
        deps_root / "cmake" / "ZXing" / "ZXing-config.cmake",
        deps_root / "share" / "cmake" / "ZXing" / "ZXingConfig.cmake",
        deps_root / "share" / "cmake" / "ZXing" / "ZXing-config.cmake",
        deps_root / "share" / "cmake" / "zxing-cpp" / "zxing-cpp-config.cmake",
        deps_root / "share" / "zxing-cpp" / "zxing-cpp-config.cmake",
        deps_root / "share" / "ZXing" / "ZXingConfig.cmake",
    ]

    missing_yaml = not any_path(yaml_configs)
    missing_zxing = not any_path(zxing_configs)
    if not (missing_yaml or missing_zxing):
        return

    deps_script = repo_root / "libs" / "build_deps_windows.ps1"
    require_path(deps_script, f"{deps_script} is required to build dependencies.")

    if deps_root.resolve() != default_root.resolve():
        if missing_yaml:
            error("yaml-cpp config missing in DepsRoot. Populate prebuilt deps or use default cache.")
        if missing_zxing:
            error("ZXing config missing in DepsRoot. Populate prebuilt deps or use default cache.")
        sys.exit(1)

        if missing_yaml:
            ensure_dependency(yaml_configs, deps_script, timeout, env, force_x64=True)
        if missing_zxing:
            ensure_dependency(zxing_configs, deps_script, timeout, env, force_x64=True)


def ensure_protobuf(deps_root: Path, timeout: int, env: dict, repo_root: Path) -> None:
    if protobuf_installed(deps_root):
        info(f"Using cached protobuf from {deps_root}")
        return
    proto_script = repo_root / "libs" / "build_protobuf_windows.ps1"
    ensure_dependency([], proto_script, timeout, env)


def public_resource_names(repo_root: Path) -> list[str]:
    public_dir = repo_root / "res" / "public"
    if not public_dir.exists():
        return []
    return [path.name for path in public_dir.iterdir() if path.is_file()]


def resource_cache_dir(repo_root: Path) -> Path:
    return repo_root / "build" / "resource_cache"


def resource_cache_ready(cache_dir: Path) -> bool:
    if not cache_dir.exists():
        return False
    return all((cache_dir / name).exists() for name in RESOURCE_FILES)


def copy_cache_resources(cache_dir: Path, release_dir: Path) -> None:
    release_dir.mkdir(parents=True, exist_ok=True)
    for item in cache_dir.iterdir():
        if item.is_file():
            shutil.copy2(item, release_dir / item.name)


def update_resource_cache(cache_dir: Path, release_dir: Path, public_names: list[str]) -> None:
    cache_dir.mkdir(parents=True, exist_ok=True)
    for name in RESOURCE_FILES.union(public_names):
        src = release_dir / name
        if src.exists():
            shutil.copy2(src, cache_dir / name)


def core_cache_dir(repo_root: Path) -> Path:
    return repo_root / "build" / "core_cache"


def release_dir_for(build_dir: Path, config: str, use_ninja: bool) -> Path:
    path = build_dir / config
    if use_ninja and not path.exists():
        path = build_dir
    return path


def ensure_core(release_dir: Path, repo_root: Path, force_download: bool) -> None:
    dest = release_dir / CORE_NAME
    if dest.exists() and not force_download:
        info(f"{CORE_NAME} already present.")
        return
    cached = core_cache_dir(repo_root) / CORE_NAME
    if cached.exists() and not force_download:
        info("Restoring cached nekobox_core...")
        release_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(cached, dest)
        return
    if download_core(repo_root, release_dir):
        release_dir.mkdir(parents=True, exist_ok=True)
        cache_dir = core_cache_dir(repo_root)
        cache_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(dest, cache_dir / CORE_NAME)
        return
    # Try building from source before attempting download
    build_core_script = repo_root / "libs" / "build_core_windows.ps1"
    if build_core_script.exists():
        info("Building nekobox_core from source...")
        try:
            run_command(
                ["powershell", "-ExecutionPolicy", "Bypass", "-File", str(build_core_script), "-DestDir", str(release_dir)]
            )
            if dest.exists():
                cache_dir = core_cache_dir(repo_root)
                cache_dir.mkdir(parents=True, exist_ok=True)
                shutil.copy2(dest, cache_dir / CORE_NAME)
                return
        except subprocess.CalledProcessError:
            print("[WARN] Building nekobox_core from source failed; will try downloading.")
    if download_core(repo_root, release_dir):
        release_dir.mkdir(parents=True, exist_ok=True)
        cache_dir = core_cache_dir(repo_root)
        cache_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(dest, cache_dir / CORE_NAME)
        return
    error(f"Failed to ensure {CORE_NAME}; please provide it at {release_dir}")
    sys.exit(1)


RESOURCE_FILES = {"geoip.dat", "geosite.dat", "geoip.db", "geosite.db"}
CORE_NAME = "nekobox_core.exe"


def find_vs_dev_cmd() -> Path | None:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        return None
    result = run_command([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild", "-property", "installationPath"], capture=True)
    path = result.stdout.strip()
    if not path:
        return None
    dev_cmd = Path(path) / "Common7" / "Tools" / "VsDevCmd.bat"
    return dev_cmd if dev_cmd.exists() else None


def import_vs_env(vs_dev_cmd: Path) -> dict:
    cmd = ["cmd", "/c", f"\"{vs_dev_cmd}\" -no_logo && set"]
    result = run_command(cmd, capture=True)
    env = os.environ.copy()
    for line in result.stdout.splitlines():
        if "=" in line:
            key, val = line.split("=", 1)
            env[key] = val
    return env


def any_path(paths: list[Path]) -> bool:
    return any(path.exists() for path in paths)


def call_ps_script(script: Path, timeout: int, env: dict | None = None) -> None:
    if not script.exists():
        error(f"Script not found: {script}")
        sys.exit(1)
    run_command(["powershell", "-ExecutionPolicy", "Bypass", "-File", str(script)], timeout=timeout, env=env)


def ensure_dependency(
    configs: list[Path], script: Path, timeout: int, env: dict | None = None, force_x64: bool = False
) -> None:
    if configs and any_path(configs):
        return
    info(f"Dependency missing; invoking {script.name}...")
    try:
        target_env = env
        if force_x64 and env is not None:
            target_env = prepare_x64_env(env)
        call_ps_script(script, timeout, env=target_env)
    except subprocess.TimeoutExpired:
        error(f"Dependency build timed out after {timeout}s.")
        sys.exit(1)


def protobuf_installed(deps_root: Path) -> bool:
    paths_to_check = [
        deps_root / "bin" / "protoc.exe",
        deps_root / "tools" / "protobuf" / "protoc.exe",
    ]
    if not any_path(paths_to_check):
        return False
    libs = [
        deps_root / "lib" / "libprotobuf.lib",
        deps_root / "lib" / "protobuf.lib",
    ]
    configs = [
        deps_root / "cmake" / "protobuf" / "protobuf-config.cmake",
        deps_root / "cmake" / "protobuf" / "protobufConfig.cmake",
        deps_root / "share" / "protobuf" / "protobuf-config.cmake",
        deps_root / "share" / "protobuf" / "protobufConfig.cmake",
    ]
    return any_path(libs) and any_path(configs)


def configure_cmake(
    repo_root: Path,
    build_dir: Path,
    qt_root: Path,
    deps_root: Path,
    disable_grpc: bool,
    generator: str,
    env: dict,
) -> None:
    cmake_args = [
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
    if disable_grpc:
        cmake_args.append("-DNKR_NO_GRPC=ON")
    if generator:
        cmake_args = ["cmake", "-G", generator] + cmake_args[1:]
    run_command(cmake_args, env=env)


def build_project(build_dir: Path, config: str, use_ninja: bool, env: dict) -> None:
    info(f"Building (Config={config})...")
    build_args = ["cmake", "--build", str(build_dir), "--config", config]
    if not use_ninja:
        build_args.extend(["--", "/m:1", "/p:TrackFileAccess=false", "/p:EnableMinimalRebuild=false"])
    run_command(build_args, env=env)


def copy_executable(release_dir: Path, repo_root: Path) -> None:
    built = release_dir / "nekobox.exe"
    require_path(built, f"Built exe not found at {built}")
    target = repo_root / "nekobox.exe"
    info(f"Copying to repo root: {target}")
    shutil.copy2(built, target)


def download_resources(repo_root: Path, release_dir: Path) -> None:
    script = repo_root / "libs" / "download_resources.ps1"
    cache_dir = resource_cache_dir(repo_root)
    public_names = public_resource_names(repo_root)
    if resource_cache_ready(cache_dir):
        info("Reusing cached public resources...")
        copy_cache_resources(cache_dir, release_dir)
        return
    if not script.exists():
        print("[WARN] download_resources.ps1 missing; skipping.")
        return
    info("Downloading public resources...")
    try:
        run_command(
            ["powershell", "-ExecutionPolicy", "Bypass", "-File", str(script), "-DestDir", str(release_dir)]
        )
        update_resource_cache(cache_dir, release_dir, public_names)
    except subprocess.CalledProcessError:
        print("[WARN] Resource download failed; continuing.")


def download_core(repo_root: Path, release_dir: Path) -> bool:
    script = repo_root / "libs" / "download_core.ps1"
    if not script.exists():
        print("[WARN] download_core.ps1 missing; skipping nekobox_core download.")
        return False
    info("Downloading nekobox_core...")
    try:
        run_command(
            ["powershell", "-ExecutionPolicy", "Bypass", "-File", str(script), "-DestDir", str(release_dir)]
        )
    except subprocess.CalledProcessError:
        print("[WARN] Nekobox core download failed; continuing.")
        return False
    return (release_dir / CORE_NAME).exists()


def main() -> None:
    parser = argparse.ArgumentParser(description="Minimal Windows build driver for Nekoray.")
    parser.add_argument("-QtRoot", default="", help="Qt SDK root directory")
    parser.add_argument("-Config", choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"], default="Release")
    parser.add_argument("-DepsRoot", default="", help="Prebuilt dependency root")
    parser.add_argument("-DisableGrpc", action="store_true")
    parser.add_argument("-DepsBuildTimeoutSec", type=int, default=120)
    parser.add_argument("-DownloadCore", action="store_true", help="Force re-download of nekobox_core even if cached")
    parser.add_argument("-DownloadResources", dest="DownloadResources", action="store_true")
    parser.add_argument("-NoDownloadResources", dest="DownloadResources", action="store_false")
    parser.set_defaults(DownloadResources=True)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent
    qt_root = require_path(Path(args.QtRoot or repo_root / "qt_lib" / "qt650"), "QtRoot does not exist.")
    info(f"Using QtRoot: {qt_root}")

    run_command(["cmake", "--version"])

    use_ninja, generator, env = detect_generator()
    build_dir = repo_root / ("build_ninja" if use_ninja else "build")

    default_deps = repo_root / "libs" / "deps" / "built"
    deps_root = require_path(determine_deps_root(args.DepsRoot, repo_root), f"DepsRoot '{args.DepsRoot}' does not exist.")
    info(f"Using DepsRoot: {deps_root}")

    ensure_dependencies(deps_root, default_deps, repo_root, args.DepsBuildTimeoutSec, env)

    if not args.DisableGrpc:
        ensure_protobuf(deps_root, args.DepsBuildTimeoutSec, prepare_x64_env(env), repo_root)
    else:
        info("gRPC disabled; skipping protobuf build.")

    kill_process_by_name("nekobox")

    configure_cmake(repo_root, build_dir, qt_root, deps_root, args.DisableGrpc, generator, env)
    build_project(build_dir, args.Config, use_ninja, env)
    release_dir = release_dir_for(build_dir, args.Config, use_ninja)
    copy_executable(release_dir, repo_root)
    ensure_core(release_dir, repo_root, args.DownloadCore)
    if args.DownloadResources:
        download_resources(repo_root, release_dir)

    info("Build completed successfully.")


if __name__ == "__main__":
    main()
