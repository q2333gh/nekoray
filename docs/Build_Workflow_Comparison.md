# 构建流程对比：build_windows.py vs CI Workflow

本文档对比本地构建脚本 `build_windows.py` 和 GitHub Actions CI workflow (`.github/workflows/windows-build.yml`) 的构建流程异同。

## 📋 概述

| 特性 | build_windows.py | CI Workflow |
|------|------------------|-------------|
| **用途** | 本地开发构建 | 自动化 CI/CD 构建 |
| **环境** | 开发者本地机器 | GitHub Actions Windows Runner |
| **Qt 来源** | 需要预先准备 `qt_lib/qt650` | 使用 `aqtinstall` 自动安装 |
| **依赖处理** | 构建所有依赖库 | 禁用大部分依赖（使用系统库） |
| **输出** | 本地可执行文件 | Release 压缩包 + GitHub Release |

## 🔄 构建流程对比

### build_windows.py 流程

```
1. 检测构建工具链（MSVC/Ninja）
   ↓
2. 验证 Qt SDK 路径（qt_lib/qt650）
   ↓
3. 构建第三方依赖库
   - yaml-cpp (从源码构建)
   - zxing-cpp (从源码构建)
   - protobuf (从源码构建)
   - QHotkey (子模块)
   ↓
4. CMake 配置
   - 启用所有依赖库
   - 使用本地构建的依赖库路径
   ↓
5. CMake 构建
   ↓
6. CMake 安装
   ↓
7. 复制可执行文件到项目根目录
   ↓
8. 后处理
   - windeployqt (部署 Qt DLL)
   - 复制 OpenSSL DLL
   - 下载资源文件（geoip.dat, geosite.dat 等）
   - 下载 nekobox_core.exe（可选）
```

### CI Workflow 流程

```
1. Checkout 代码仓库
   ↓
2. 设置 Python 环境
   ↓
3. 安装 aqtinstall
   ↓
4. 安装 MSVC 编译器工具链
   ↓
5. 使用 aqtinstall 安装 Qt 6.5.3
   ↓
6. CMake 配置
   - 禁用大部分依赖（NO_GRPC, NO_YAML, NO_ZXING, NO_QHOTKEY）
   - 使用系统提供的库或最小化依赖
   ↓
7. CMake 构建
   ↓
8. 获取版本信息（从 git commit date）
   ↓
9. windeployqt (部署 Qt DLL)
   ↓
10. 复制 OpenSSL DLL
    ↓
11. 下载资源文件（geoip.dat, geosite.dat 等）
    ↓
12. 下载 nekobox_core.exe（从上游仓库）
    ↓
13. 打包 Release 压缩包
    ↓
14. 创建 GitHub Release 并上传
```

## 🔍 主要差异

### 1. Qt SDK 处理

**build_windows.py:**
- 要求 Qt SDK 预先解压到 `qt_lib/qt650` 目录
- 使用本地路径：`qt_lib/qt650`
- 需要手动下载和解压 Qt SDK

**CI Workflow:**
- 使用 `aqtinstall` 自动下载和安装 Qt
- Qt 安装到：`C:\Qt\6.5.3\msvc2019_64`
- 完全自动化，无需手动准备

### 2. 依赖库处理

**build_windows.py:**
```python
# 构建所有依赖库
ensure_third_party()  # yaml-cpp, zxing-cpp
ensure_protobuf()     # protobuf
ensure_qhotkey()      # QHotkey

# CMake 配置启用所有依赖
cmake_configure([
    "-DNKR_LIBS={deps_root}",  # 使用本地构建的依赖
    ...
])
```

**CI Workflow:**
```yaml
# 禁用大部分依赖，使用系统库或最小化配置
cmake -S . -B build
  -DNKR_NO_GRPC=ON
  -DNKR_NO_YAML=ON
  -DNKR_NO_ZXING=ON
  -DNKR_NO_QHOTKEY=ON
```

**原因：**
- CI 环境需要快速构建，避免编译大量依赖库
- 使用系统提供的库或静态链接减少依赖
- 本地开发需要完整功能，包括所有可选依赖

### 3. 构建工具链

**build_windows.py:**
- 自动检测本地安装的 MSVC 和 Ninja
- 支持多种生成器（Visual Studio, Ninja）
- 使用 `vswhere` 查找 Visual Studio

**CI Workflow:**
- 使用 GitHub Actions 提供的 MSVC 工具链
- 通过 `ilammy/msvc-dev-cmd@v1` 设置环境
- 固定使用默认工具链

### 4. 版本信息

**build_windows.py:**
- 使用 `nekoray_version.txt` 中的版本号
- 不修改版本信息

**CI Workflow:**
- 从 git commit date 生成版本号
- 格式：`{base_version}-{commit_date}-{build_number}`
- 例如：`v4.0.1-2026-01-14-50`

### 5. 输出产物

**build_windows.py:**
- `nekobox.exe` (项目根目录)
- `build/Release/nekobox.exe` (构建目录)
- 完整的运行时文件（DLL、资源文件等）

**CI Workflow:**
- `nekoray-win64-{version}-{build}.zip` (Release 压缩包)
- 上传到 GitHub Release
- 包含所有运行时依赖

### 6. 资源文件处理

**build_windows.py:**
- 通过 `platform.post_build()` 调用资源下载脚本
- 下载到 `build/Release/` 目录

**CI Workflow:**
- 显式调用 `libs/download_resources.py`
- 下载到 `build/Release/` 目录
- 包含在最终的 Release 压缩包中

### 7. nekobox_core.exe 处理

**build_windows.py:**
- 可选下载，从 `MatsuriDayo/nekoray` releases
- 如果下载失败，构建仍可继续

**CI Workflow:**
- 必须下载，从上游仓库的 releases
- 如果下载失败，会显示警告但继续构建
- 最终 Release 压缩包可能不包含 core（如果下载失败）

## 📊 CMake 配置参数对比

### build_windows.py
```cmake
-DQT_VERSION_MAJOR=6
-DNKR_LIBS={deps_root}              # 使用本地构建的依赖库
-DCMAKE_PREFIX_PATH={qt_root}
-DCMAKE_VS_GLOBALS=TrackFileAccess=false;EnableMinimalRebuild=false
-DQT_NO_PACKAGE_VERSION_CHECK=TRUE
```

### CI Workflow
```cmake
-DQT_VERSION_MAJOR=6
-DNKR_NO_GRPC=ON                    # 禁用 gRPC
-DNKR_NO_YAML=ON                    # 禁用 yaml-cpp
-DNKR_NO_ZXING=ON                   # 禁用 zxing-cpp
-DNKR_NO_QHOTKEY=ON                 # 禁用 QHotkey
-DCMAKE_PREFIX_PATH={qt_root}
```

## ⚙️ 环境要求

### build_windows.py
- Python 3.x
- Visual Studio 2019/2022（已安装）
- CMake（已安装）
- Qt 6.5 SDK（预先解压到 `qt_lib/qt650`）
- Git（用于克隆依赖库）
- Ninja（可选，但推荐）

### CI Workflow
- GitHub Actions Windows Runner
- Python 3.x（自动安装）
- MSVC 编译器（自动配置）
- Qt 6.5.3（通过 aqtinstall 自动安装）
- CMake（Runner 自带）

## 🎯 使用场景

### 使用 build_windows.py
- ✅ 本地开发和调试
- ✅ 需要完整功能（包括所有可选依赖）
- ✅ 需要频繁构建和测试
- ✅ 需要自定义构建配置

### 使用 CI Workflow
- ✅ 自动化构建和发布
- ✅ 生成 Release 压缩包
- ✅ 持续集成验证
- ✅ 无需本地环境准备

## 🔧 故障排查

### build_windows.py 常见问题
1. **Qt SDK 未找到**
   - 确保 Qt SDK 已解压到 `qt_lib/qt650`
   - 检查路径是否正确

2. **依赖库构建失败**
   - 检查网络连接（需要下载源码）
   - 检查 Visual Studio 是否正确安装
   - 查看构建日志了解具体错误

### CI Workflow 常见问题
1. **Qt 安装失败**
   - 检查 `aqtinstall` 版本是否支持 Qt 6.5.3
   - 检查架构名称是否正确（`win64_msvc2019_64`）

2. **Checkout 失败**
   - 确保 `submodules: false`（避免未配置的子模块）
   - 检查仓库权限

3. **构建失败**
   - 查看 GitHub Actions 日志
   - 检查 CMake 配置参数是否正确

## 📝 总结

两个构建流程服务于不同的目的：

- **build_windows.py**：完整的本地开发构建流程，构建所有依赖库，适合开发和调试
- **CI Workflow**：快速自动化构建流程，最小化依赖，专注于生成 Release 产物

两者在核心构建步骤上相似，但在依赖处理、环境准备和输出产物方面有显著差异。理解这些差异有助于选择合适的构建方式。
