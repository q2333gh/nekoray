#!/usr/bin/env python3
"""Minimal Python-only build driver that replaces the old PowerShell helpers."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

CONFIG = "Release"
QT_REL_PATH = Path("qt_lib/qt650")
DEPS_ROOT = Path("libs/deps/built")
DEPS_CACHE_DIR = Path("libs/deps")
DEPS_BUILD_TIMEOUT = 900
DOWNLOAD_TIMEOUT = 300
CORE_URL = "https://github.com/MatsuriDayo/nekoray/releases"


def info(msg: str) -> None:
    print(f"[INFO] {msg}")


def error(msg: str) -> None:
    print(f"[ERROR] {msg}", file=sys.stderr)


def run_command(cmd, *, env=None, timeout=None, capture=False):
    info("Running: " + " ".join(str(part) for part in cmd))
    try:
        return subprocess.run(
            cmd,
            check=True,
            env=env,
            text=True,
            timeout=timeout,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.STDOUT if capture else None,
        )
    except subprocess.CalledProcessError as exc:
        if capture and exc.stdout:
            error(exc.stdout)
        error(f"Command failed (exit {exc.returncode}): {' '.join(cmd)}")
        raise
    except subprocess.TimeoutExpired as exc:
        error(f"Command timed out after {timeout}s: {' '.join(cmd)}")
        raise


def require_path(path: Path, message: str) -> Path:
    if not path.exists():
        error(message)
        sys.exit(1)
    return path


def any_path(paths: list[Path]) -> bool:
    return any(path.exists() for path in paths)


def find_vs_dev_cmd() -> Path | None:
    vswhere = (
        Path(os.environ.get("ProgramFiles(x86)", ""))
        / "Microsoft Visual Studio"
        / "Installer"
        / "vswhere.exe"
    )
    if not vswhere.exists():
        return None
    result = run_command(
        [
            str(vswhere),
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.Component.MSBuild",
            "-property",
            "installationPath",
        ],
        capture=True,
    )
    base = Path(result.stdout.strip()) if result.stdout else None
    if not base:
        return None
    candidate = base / "Common7" / "Tools" / "VsDevCmd.bat"
    return candidate if candidate.exists() else None


def import_vs_env(vs_dev_cmd: Path) -> dict:
    result = run_command(["cmd", "/c", f'"{vs_dev_cmd}" -no_logo && set'], capture=True)
    env = os.environ.copy()
    for line in result.stdout.splitlines():
        if "=" in line:
            key, val = line.split("=", 1)
            env[key] = val
    return env


def detect_generator() -> tuple[bool, str, dict]:
    env = os.environ.copy()
    ninja = shutil.which("ninja")
    if not ninja:
        return False, "", env
    vs_dev_cmd = find_vs_dev_cmd()
    if not vs_dev_cmd:
        info(
            "Ninja detected but VS dev environment unavailable; falling back to MSVC generator."
        )
        return False, "", env
    info("Using generator: Ninja Multi-Config")
    env = import_vs_env(vs_dev_cmd)
    return True, "Ninja Multi-Config", env


def prepare_x64_env(base_env: dict) -> dict:
    env = base_env.copy()
    env.setdefault("VSCMD_ARG_TGT_ARCH", "x64")
    env.setdefault("VSCMD_ARG_HOST_ARCH", "x64")
    env.setdefault("PreferredToolArchitecture", "x64")
    return env


def ensure_repo(path: Path, url: str, tag: str) -> None:
    if path.exists():
        return
    cmd = ["git", "clone", "--depth", "1", "-b", tag, url, str(path)]
    run_command(cmd)


def cmake_configure(
    src: Path, build: Path, install: Path, generator: str, env: dict, extra: list[str]
) -> None:
    build.mkdir(parents=True, exist_ok=True)
    args = [
        "cmake",
        "-S",
        str(src),
        "-B",
        str(build),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DCMAKE_INSTALL_PREFIX=" + str(install),
    ]
    if generator:
        args = ["cmake", "-G", generator] + args[1:]
    args.extend(extra)
    run_command(args, env=env)


def cmake_build(build: Path, env: dict, use_ninja: bool) -> None:
    import multiprocessing

    cpu_count = multiprocessing.cpu_count()
    args = ["cmake", "--build", str(build), "--config", "Release"]
    if use_ninja:
        args.extend(["--", "-j", str(cpu_count)])
    else:
        args.extend(
            [
                "--",
                f"/m:{cpu_count}",
                "/p:TrackFileAccess=false",
                "/p:EnableMinimalRebuild=false",
            ]
        )
    run_command(args, env=env)


def cmake_install(build: Path, env: dict) -> None:
    run_command(["cmake", "--install", str(build), "--config", "Release"], env=env)


def patch_yaml_cmake(src: Path) -> None:
    cmake_file = src / "CMakeLists.txt"
    if not cmake_file.exists():
        return
    content = cmake_file.read_text()
    pattern = re.compile(
        r"cmake_minimum_required\s*\(VERSION\s+[<]?\s*3\.[0-4][^)]+\)", re.IGNORECASE
    )
    if pattern.search(content):
        info("Updating yaml-cpp CMakeLists.txt minimum version to 3.5")
        content = pattern.sub("cmake_minimum_required(VERSION 3.5)", content)
        cmake_file.write_text(content)


def ensure_library(
    repo_root: Path,
    deps_root: Path,
    name: str,
    url: str,
    tag: str,
    generator: str,
    env: dict,
    use_ninja: bool,
    configs: list[Path],
    extra_args: list[str],
) -> None:
    if any_path([deps_root / rel for rel in configs]):
        return
    info(f"Building {name} from {tag}")
    src = (
        deps_root.parent / name
        if name != "protobuf"
        else repo_root / "libs" / "deps" / name
    )
    ensure_repo(src, url, tag)
    if name == "yaml-cpp":
        patch_yaml_cmake(src)
    build_dir = src / (
        "build" if name != "protobuf" else ("build-ninja" if use_ninja else "build")
    )
    install_dir = deps_root
    cmake_configure(src, build_dir, install_dir, generator, env, extra_args)
    cmake_build(build_dir, env, use_ninja)
    cmake_install(build_dir, env)


def protobuf_installed(deps_root: Path) -> bool:
    return any_path(
        [deps_root / "bin/protoc.exe", deps_root / "tools/protobuf/protoc.exe"]
    ) and any_path(
        [
            deps_root / "lib/libprotobuf.lib",
            deps_root / "lib/protobuf.lib",
            deps_root / "cmake/protobuf-config.cmake",
            deps_root / "cmake/protobuf-config-version.cmake",
            deps_root / "share/protobuf/protobuf-config.cmake",
        ]
    )


def ensure_third_party(
    repo_root: Path, deps_root: Path, generator: str, env: dict, use_ninja: bool
) -> None:
    scan = lambda rel: Path(rel)
    zxing_configs = [
        scan("lib/cmake/ZXing/ZXingConfig.cmake"),
        scan("lib/cmake/ZXing/ZXing-config.cmake"),
        scan("cmake/ZXing/ZXingConfig.cmake"),
        scan("cmake/ZXing/ZXing-config.cmake"),
        scan("share/cmake/ZXing/ZXingConfig.cmake"),
        scan("share/cmake/ZXing/ZXing-config.cmake"),
        scan("share/cmake/zxing-cpp/zxing-cpp-config.cmake"),
        scan("share/zxing-cpp/zxing-cpp-config.cmake"),
        scan("share/ZXing/ZXingConfig.cmake"),
    ]
    yaml_configs = [
        scan("lib/cmake/yaml-cpp/yaml-cpp-config.cmake"),
        scan("lib/cmake/yaml-cpp/yaml-cppConfig.cmake"),
        scan("cmake/yaml-cpp/yaml-cpp-config.cmake"),
        scan("cmake/yaml-cpp/yaml-cppConfig.cmake"),
        scan("share/cmake/yaml-cpp/yaml-cpp-config.cmake"),
        scan("share/cmake/yaml-cpp/yaml-cppConfig.cmake"),
        scan("share/yaml-cpp/yaml-cpp-config.cmake"),
        scan("share/yaml-cpp/yaml-cppConfig.cmake"),
    ]
    deps_root.mkdir(parents=True, exist_ok=True)
    deps_list = [
        ("zxing-cpp", zxing_configs, "https://github.com/nu-book/zxing-cpp", "v2.0.0"),
        ("yaml-cpp", yaml_configs, "https://github.com/jbeder/yaml-cpp", "master"),
    ]
    for name, configs, url, tag in deps_list:
        cfg_paths = [deps_root / cfg for cfg in configs]
        if any_path(cfg_paths):
            continue
        ensure_library(
            repo_root,
            deps_root,
            name,
            url,
            tag,
            generator,
            env,
            use_ninja,
            configs,
            ["-DBUILD_TESTING=OFF", "-DCMAKE_BUILD_TYPE=Release"],
        )


def ensure_protobuf(
    repo_root: Path, deps_root: Path, generator: str, env: dict, use_ninja: bool
) -> None:
    if protobuf_installed(deps_root):
        info("Protobuf already cached")
        return
    ensure_library(
        repo_root,
        deps_root,
        "protobuf",
        "https://github.com/protocolbuffers/protobuf",
        "v21.4",
        generator,
        prepare_x64_env(env),
        use_ninja,
        [],
        [
            "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF",
            "-Dprotobuf_BUILD_TESTS=OFF",
            "-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false",
        ],
    )


def release_dir(build_dir: Path, use_ninja: bool) -> Path:
    return build_dir if use_ninja else build_dir / CONFIG


def copy_executable(build_dir: Path, repo_root: Path, use_ninja: bool) -> None:
    target = release_dir(build_dir, use_ninja) / "nekobox.exe"
    require_path(target, "Built executable not found")
    dest = repo_root / "nekobox.exe"
    shutil.copy2(target, dest)


def download_resources(repo_root: Path, build_dir: Path) -> None:
    script = repo_root / "libs" / "download_resources.ps1"
    if not script.exists():
        info("download_resources.ps1 missing; skipping resources")
        return
    info("Downloading public resources")
    try:
        run_command(
            [
                "powershell",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(script),
                "-DestDir",
                str(release_dir(build_dir, False)),
            ],
            timeout=DOWNLOAD_TIMEOUT,
        )
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        info("Resource download failed; continuing.")


def download_core(repo_root: Path, build_dir: Path) -> None:
    script = repo_root / "libs" / "download_core.ps1"
    if not script.exists():
        info("download_core.ps1 missing; skipping core download")
        return
    info("Downloading nekobox_core")
    try:
        run_command(
            [
                "powershell",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(script),
                "-DestDir",
                str(release_dir(build_dir, False)),
            ],
            timeout=DOWNLOAD_TIMEOUT,
        )
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        info("nekobox_core download failed; continuing.")


def main() -> None:
    repo_root = Path(__file__).resolve().parent
    qt_root = require_path(
        repo_root / QT_REL_PATH, f"Qt directory missing at {QT_REL_PATH}"
    )

    use_ninja, generator, env = detect_generator()
    build_dir = repo_root / ("build_ninja" if use_ninja else "build")
    deps_root = repo_root / DEPS_ROOT
    deps_root.parent.mkdir(parents=True, exist_ok=True)

    ensure_third_party(repo_root, deps_root, generator, env, use_ninja)
    ensure_protobuf(repo_root, deps_root, generator, env, use_ninja)

    cmake_configure(
        repo_root,
        build_dir,
        build_dir,  # install prefix (not used for main project, but required by function signature)
        generator,
        env,
        [
            "-DQT_VERSION_MAJOR=6",
            f"-DNKR_LIBS={deps_root}",
            f"-DCMAKE_PREFIX_PATH={qt_root}",
            "-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false",
        ],
    )
    cmake_build(build_dir, env, use_ninja)
    cmake_install(build_dir, env)

    copy_executable(build_dir, repo_root, use_ninja)
    download_resources(repo_root, build_dir)
    download_core(repo_root, build_dir)

    info("Build completed.")


if __name__ == "__main__":
    main()
