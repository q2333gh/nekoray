# Dependency Report

### Toolchain
- The Python build driver expects a Qt 6.5 SDK under `qt_lib/qt650`.
- Uses CMake, Ninja, and MSVC (detected via `vswhere`) to configure and build all components.
- For configuration details (paths, generator selection, MSVC env), see:  
  `build_windows.py`, lines 12–136.

### Built Third-Party Libraries
- Downloads and builds:
  - `YAML-cpp` (master branch)
  - `ZXing` v2.0.0
  - `Protobuf` v21.4  
- These are fetched from GitHub, built with CMake/Ninja, and installed to `libs/deps/built` before the main build.
- The process is orchestrated in `build_windows.py` (lines 176–230).
- Historical Linux build flow can be found in `build_deps_all.sh` (lines 27–50).

### Embedded C++ Helpers
- Bundled sources included at runtime:
  - `qrcodegen` (QR encoder)
  - `QvProxyConfigurator` (from qv2ray UI kit)
  - Various Qt helper headers in `3rdparty/…` (e.g., `qrcodegen.cpp`, `QvProxyConfigurator.hpp`, line 1 of each)
- These enable UI features and proxy functions without additional external dependencies.

### Public Resource Datasets
- The Windows downloader acquires the latest:
  - `geoip.dat`, `geosite.dat`, `geoip.db`, `geosite.db`
  - All files under `res/public`
- See `download_resources.ps1` (lines 22–47) for URLs and copy steps.

### Prebuilt Core Binary
- `nekobox_core.exe` is fetched from the latest GitHub release of `MatsuriDayo/nekoray`.
- Falls back to manual download if the API fetch fails.
- Download/placement handled in `download_core.ps1` (lines 2–110).

### Cache Expectations
- All dependencies are cached to `libs/deps/built`.
- Subsequent builds re-use downloaded/built content managed in `build_windows.py` (lines 154–232).

**依赖报告总结**

* **工具链**

  * 构建依赖 Qt 6.5 SDK（路径：`qt_lib/qt650`）。
  * 使用 CMake + Ninja + MSVC（通过 vswhere 检测）。
  * 具体配置逻辑在 `build_windows.py`（12–136 行）。

* **第三方库构建**

  * 自动下载并编译：`YAML-cpp`（master）、`ZXing` v2.0.0、`Protobuf` v21.4。
  * 使用 CMake/Ninja 构建，安装到 `libs/deps/built`。
  * Windows 流程在 `build_windows.py`（176–230 行），Linux 旧流程在 `build_deps_all.sh`。

* **内嵌 C++ 辅助组件**

  * 运行时直接包含源码：`qrcodegen`、`QvProxyConfigurator` 及若干 Qt 辅助头文件。
  * 用于 UI 与代理功能，无需额外外部依赖。

* **公共资源数据**

  * Windows 构建会下载最新的 `geoip/geosite` 数据（`.dat` 与 `.db`），放入 `res/public`。
  * 下载逻辑在 `download_resources.ps1`（22–47 行）。

* **预编译核心程序**

  * `nekobox_core.exe` 从 `MatsuriDayo/nekoray` 的 GitHub 最新 Release 获取，失败则手动下载。
  * 由 `download_core.ps1`（2–110 行）处理。

* **缓存机制**

  * 所有依赖缓存于 `libs/deps/built`，后续构建复用已下载/编译内容。
