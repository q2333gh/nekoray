# Nekoray 项目依赖升级计划

本文档详细列出了项目中所有依赖库和语言的当前版本、最新版本，以及升级步骤和注意事项。

**生成时间**: 2025-01-12

---

## 📋 目录

1. [编程语言版本](#编程语言版本)
2. [构建工具](#构建工具)
3. [C++ 依赖库](#c-依赖库)
4. [Go 依赖库](#go-依赖库)
5. [Qt 框架](#qt-框架)
6. [升级优先级](#升级优先级)
7. [升级步骤](#升级步骤)
8. [风险评估](#风险评估)

---

## 编程语言版本

### C++ 标准

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| C++ 标准 | C++17 | C++23 | ⚠️ 可升级 | 建议先升级到 C++20，C++23 支持可能不完整 |

**当前配置**: `CMakeLists.txt` 第 7 行
```cmake
set(CMAKE_CXX_STANDARD 17)
```

**升级建议**:
- **短期**: 升级到 C++20（编译器支持良好，特性稳定）
- **长期**: 考虑 C++23（需要验证编译器支持）

**影响文件**:
- `CMakeLists.txt` (第 7 行)

---

### Go 语言

| 模块 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| updater/go.mod | Go 1.18 | Go 1.25 | ⚠️ 需要升级 | 升级到 Go 1.25 |
| nekobox_core/go.mod | Go 1.19 | Go 1.25 | ⚠️ 需要升级 | 升级到 Go 1.25 |
| grpc_server/go.mod | Go 1.19 | Go 1.25 | ⚠️ 需要升级 | 升级到 Go 1.25 |

**升级步骤**:
1. 更新所有 `go.mod` 文件中的 `go` 版本声明
2. 运行 `go mod tidy` 更新依赖
3. 测试编译和运行

**影响文件**:
- `go/cmd/updater/go.mod` (第 3 行)
- `go/cmd/nekobox_core/go.mod` (第 3 行)
- `go/grpc_server/go.mod` (第 3 行)

---

## 构建工具

### CMake

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| CMake 最低要求 | 3.16 | 3.28.0 | ⚠️ 可升级 | 升级到 3.28.0 |

**当前配置**: `CMakeLists.txt` 第 1 行
```cmake
cmake_minimum_required(VERSION 3.16)
```

**升级建议**: 
- 升级到 3.28.0 以获得最新功能和性能改进
- 注意：需要确保构建环境支持新版本

**影响文件**:
- `CMakeLists.txt` (第 1 行)

---

## C++ 依赖库

### Qt 框架

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| Qt | 6.5 | 6.9.3 (或 6.8 LTS) | ⚠️ 可升级 | 建议升级到 6.8 LTS（长期支持） |

**当前配置**: 
- `CMakeLists.txt` 第 52-56 行
- `build_windows.py` 第 16 行: `QT_REL_PATH = Path("qt_lib/qt650")`

**升级建议**:
- **推荐**: 升级到 Qt 6.8 LTS（支持到 2029年）
- **备选**: 升级到 Qt 6.9.3（最新功能，但支持到 2026年3月）

**注意事项**:
- 需要下载新的 Qt SDK
- 可能需要更新 QML 代码以兼容新版本
- 测试所有 UI 功能

**影响文件**:
- `CMakeLists.txt` (第 52-56 行)
- `build_windows.py` (第 16 行，需要更新 Qt SDK 路径)

---

### Protocol Buffers (protobuf)

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| protobuf | v21.4 | v6.33 | ⚠️ 需要升级 | ⚠️ **重大版本升级，需谨慎** |

**当前配置**:
- `libs/build_deps_all.sh` 第 57 行: `v21.4`
- `build_windows.py` 第 421 行: `protobuf_tag = "v21.4"`

**升级注意事项**:
- ⚠️ **这是重大版本升级**（v21.4 → v6.33），可能有破坏性变更
- 需要检查 protobuf 定义文件（`.proto`）的兼容性
- 需要更新 Go 和 C++ 的 protobuf 库版本
- 建议先在测试环境验证

**影响文件**:
- `libs/build_deps_all.sh` (第 57 行)
- `build_windows.py` (第 421 行)
- `go/grpc_server/go.mod` (protobuf 相关依赖)

---

### yaml-cpp

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| yaml-cpp | 0.7.0 / master | 0.8.0 | ⚠️ 需要升级 | 升级到 0.8.0 |

**当前配置**:
- `libs/build_deps_all.sh` 第 44 行: `yaml-cpp-0.7.0`
- `build_windows.py` 第 398 行: `"master"` (应改为具体版本)

**升级建议**:
- 升级到 0.8.0（稳定版本）
- 统一版本管理（Linux 和 Windows 使用相同版本）

**影响文件**:
- `libs/build_deps_all.sh` (第 44 行)
- `build_windows.py` (第 398 行)

---

### ZXing-C++

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| zxing-cpp | v2.0.0 | v2.3.0 | ⚠️ 需要升级 | 升级到 v2.3.0 |

**当前配置**:
- `libs/build_deps_all.sh` 第 31 行: `v2.0.0`
- `build_windows.py` 第 397 行: `"v2.0.0"`

**升级建议**:
- 升级到 v2.3.0（包含 bug 修复和新功能）

**影响文件**:
- `libs/build_deps_all.sh` (第 31 行)
- `build_windows.py` (第 397 行)

---

### QHotkey

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| QHotkey | master (最新) | master | ✅ 已是最新 | 保持使用 master 分支 |

**当前配置**: `build_windows.py` 第 336-353 行（从 GitHub 克隆最新 master）

**建议**: 保持现状，或考虑固定到特定版本标签以提高可重现性

---

## Go 依赖库

### gRPC 相关

| 项目 | 当前版本 | 最新版本 | 状态 | 建议 |
|------|---------|---------|------|------|
| google.golang.org/grpc | 1.49.0 (grpc_server) / 1.63.2 (nekobox_core) | 1.76.0 | ⚠️ 需要升级 | 统一升级到 1.76.0 |
| google.golang.org/protobuf | 1.28.1 (grpc_server) / 1.33.0 (nekobox_core) | 1.36.10 | ⚠️ 需要升级 | 统一升级到 1.36.10 |
| github.com/grpc-ecosystem/go-grpc-middleware | 1.3.0 | 2.3.3 | ⚠️ 需要升级 | ⚠️ **重大版本升级** |

**当前配置**:
- `go/grpc_server/go.mod`:
  - `google.golang.org/grpc v1.49.0`
  - `google.golang.org/protobuf v1.28.1`
  - `github.com/grpc-ecosystem/go-grpc-middleware v1.3.0`
- `go/cmd/nekobox_core/go.mod`:
  - `google.golang.org/grpc v1.63.2`
  - `google.golang.org/protobuf v1.33.0`

**升级注意事项**:
- ⚠️ `go-grpc-middleware` 从 v1.x 升级到 v2.x 可能有 API 变更
- 需要统一所有模块的 gRPC 和 protobuf 版本
- 建议先升级 grpc_server，然后升级 nekobox_core

**影响文件**:
- `go/grpc_server/go.mod`
- `go/cmd/nekobox_core/go.mod`

---

### 其他 Go 依赖

大部分 Go 依赖在 `nekobox_core/go.mod` 中，建议运行 `go get -u ./...` 更新所有依赖到最新兼容版本。

**主要依赖**:
- `github.com/sagernet/sing-box` (replaced，使用本地路径)
- `github.com/matsuridayo/libneko` (replaced，使用本地路径)
- 其他间接依赖（约 80+ 个）

**升级建议**:
1. 先升级直接依赖
2. 运行 `go mod tidy` 更新间接依赖
3. 测试编译和功能

---

## 升级优先级

### 🔴 高优先级（安全性和稳定性）

1. **Go 语言版本** (1.18/1.19 → 1.25)
   - 原因：获得安全更新和性能改进
   - 风险：低
   - 工作量：小

2. **yaml-cpp** (0.7.0 → 0.8.0)
   - 原因：bug 修复和稳定性改进
   - 风险：低
   - 工作量：小

3. **ZXing-C++** (v2.0.0 → v2.3.0)
   - 原因：bug 修复
   - 风险：低
   - 工作量：小

### 🟡 中优先级（功能改进）

4. **Qt** (6.5 → 6.8 LTS)
   - 原因：长期支持，新功能
   - 风险：中（需要测试 UI）
   - 工作量：中

5. **CMake** (3.16 → 3.28.0)
   - 原因：新功能和性能改进
   - 风险：低
   - 工作量：小

6. **C++ 标准** (C++17 → C++20)
   - 原因：现代 C++ 特性
   - 风险：中（需要代码审查）
   - 工作量：中

### 🟢 低优先级（需要谨慎）

7. **Protocol Buffers** (v21.4 → v6.33)
   - 原因：重大版本升级，可能有破坏性变更
   - 风险：高
   - 工作量：大
   - **建议**: 先在独立分支测试

8. **gRPC 中间件** (v1.3.0 → v2.3.3)
   - 原因：重大版本升级
   - 风险：中
   - 工作量：中
   - **建议**: 与 protobuf 升级一起进行

---

## 升级步骤

### 阶段 1: 低风险升级（1-2 天）

```bash
# 1. 升级 Go 版本
# 编辑 go/cmd/updater/go.mod
go 1.25

# 编辑 go/cmd/nekobox_core/go.mod
go 1.25

# 编辑 go/grpc_server/go.mod
go 1.25

# 2. 更新 Go 依赖
cd go/cmd/updater && go mod tidy
cd ../nekobox_core && go mod tidy
cd ../../grpc_server && go mod tidy

# 3. 升级 yaml-cpp 和 ZXing
# 编辑 libs/build_deps_all.sh 和 build_windows.py
# yaml-cpp: 0.7.0 → 0.8.0
# zxing-cpp: v2.0.0 → v2.3.0

# 4. 升级 CMake 最低版本
# 编辑 CMakeLists.txt: cmake_minimum_required(VERSION 3.28)
```

### 阶段 2: 中风险升级（3-5 天）

```bash
# 1. 升级 Qt 到 6.8 LTS
# - 下载新的 Qt SDK
# - 更新 build_windows.py 中的路径
# - 测试所有 UI 功能

# 2. 升级 C++ 标准到 C++20
# 编辑 CMakeLists.txt: set(CMAKE_CXX_STANDARD 20)
# - 编译测试
# - 修复可能的兼容性问题
```

### 阶段 3: 高风险升级（1-2 周）

```bash
# 1. 创建特性分支
git checkout -b upgrade/protobuf-grpc

# 2. 升级 Protocol Buffers
# - 更新 libs/build_deps_all.sh: v21.4 → v6.33
# - 更新 build_windows.py: protobuf_tag = "v6.33"
# - 重新生成 .proto 文件
# - 测试所有 protobuf 相关功能

# 3. 升级 gRPC 相关库
# - 更新 go/grpc_server/go.mod
# - 更新 go/cmd/nekobox_core/go.mod
# - 检查 API 变更
# - 更新代码以适配新 API

# 4. 全面测试
# - 单元测试
# - 集成测试
# - 端到端测试
```

---

## 风险评估

### 低风险升级 ✅

- Go 语言版本升级
- yaml-cpp 升级
- ZXing-C++ 升级
- CMake 版本升级

**应对措施**: 直接升级，运行测试套件

### 中风险升级 ⚠️

- Qt 升级
- C++ 标准升级
- gRPC 中间件升级

**应对措施**:
- 在特性分支进行
- 充分测试 UI 和功能
- 代码审查

### 高风险升级 🔴

- Protocol Buffers 重大版本升级

**应对措施**:
- 创建独立测试分支
- 详细测试所有 protobuf 相关功能
- 准备回滚方案
- 分阶段升级（先测试环境，再生产环境）

---

## 测试清单

升级后必须测试的功能：

### 核心功能
- [ ] 代理配置加载和保存
- [ ] 代理连接和断开
- [ ] 订阅更新
- [ ] 配置文件导入/导出

### UI 功能
- [ ] 主窗口显示
- [ ] 设置界面
- [ ] 代理列表
- [ ] 日志显示
- [ ] 翻译功能

### 平台特定
- [ ] Windows 构建
- [ ] Linux 构建
- [ ] 热键功能（QHotkey）
- [ ] 系统托盘

### 集成测试
- [ ] gRPC 通信
- [ ] Protobuf 序列化/反序列化
- [ ] YAML 配置解析
- [ ] QR 码生成（ZXing）

---

## 回滚计划

如果升级出现问题，按以下步骤回滚：

1. **Git 回滚**
   ```bash
   git checkout <previous-commit>
   ```

2. **依赖回滚**
   - 恢复 `go.mod` 文件
   - 恢复构建脚本中的版本号
   - 清理构建缓存：`rm -rf libs/deps/built`

3. **重新构建**
   ```bash
   python build_windows.py  # Windows
   # 或
   ./libs/build_deps_all.sh  # Linux
   ```

---

## 维护建议

1. **版本固定**: 考虑将依赖固定到具体版本标签，而不是使用 `master` 分支
2. **定期更新**: 每季度检查一次依赖更新
3. **自动化测试**: 确保有完整的测试套件覆盖核心功能
4. **文档更新**: 升级后更新 README 和构建文档

---

## 参考资源

- [Qt 6.8 LTS 发布说明](https://www.qt.io/blog/qt-6.8-released)
- [Protocol Buffers 升级指南](https://protobuf.dev/news/)
- [Go 1.25 发布说明](https://go.dev/doc/devel/release)
- [CMake 3.28 发布说明](https://cmake.org/cmake/help/latest/release/3.28.html)

---

**最后更新**: 2025-01-12
**维护者**: 开发团队
