# Nekoray 本地构建完整指南

本文档总结了在Ubuntu 22.04环境下完整构建Nekoray的步骤，基于实际构建过程中遇到的问题和解决方案。

## 前置条件

### 系统环境
- Ubuntu 22.04 LTS
- 网络代理（如需要）：`export HTTP_PROXY="http://127.0.0.1:2080" HTTPS_PROXY="http://127.0.0.1:2080" ALL_PROXY="socks5://127.0.0.1:2080"`

### 安装基础构建工具

```bash
# 更新包管理器
sudo apt update

# 安装基础构建工具
sudo apt install -y cmake ninja-build pkg-config build-essential git

# 安装Qt5开发环境
sudo apt install -y qtbase5-dev qttools5-dev libqt5svg5-dev qtbase5-dev-tools libqt5x11extras5-dev

# 安装C++依赖库
sudo apt install -y libprotobuf-dev protobuf-compiler libyaml-cpp-dev
```

### 安装Go 1.22+

由于系统自带的Go版本可能过低（如1.18），需要手动安装新版本：

```bash
# 下载Go 1.22.5
export HTTP_PROXY="http://127.0.0.1:2080" HTTPS_PROXY="http://127.0.0.1:2080" ALL_PROXY="socks5://127.0.0.1:2080"
wget https://go.dev/dl/go1.22.5.linux-amd64.tar.gz

# 安装Go
sudo rm -rf /usr/local/go
sudo tar -C /usr/local -xzf go1.22.5.linux-amd64.tar.gz

# 设置PATH
export PATH=/usr/local/go/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
go version  # 验证版本
```

## 构建步骤

### 1. 获取源码

```bash
# 克隆项目（如果还没有）
git clone https://github.com/MatsuriDayo/nekoray.git --recursive
cd nekoray

# 如果已有项目，更新子模块
git submodule update --init --recursive
```

### 2. 获取Go依赖源码

```bash
# 设置代理环境变量（如需要）
export HTTP_PROXY="http://127.0.0.1:2080" HTTPS_PROXY="http://127.0.0.1:2080" ALL_PROXY="socks5://127.0.0.1:2080"

# 获取Go项目依赖源码
./libs/get_source.sh
```

### 3. 构建Go部分

```bash
# 设置环境变量
export PATH=/usr/local/go/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export HTTP_PROXY="http://127.0.0.1:2080" HTTPS_PROXY="http://127.0.0.1:2080" ALL_PROXY="socks5://127.0.0.1:2080"

# 使用bash执行构建（避免fish shell兼容性问题）
bash -c '
export PATH=/usr/local/go/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export HTTP_PROXY="http://127.0.0.1:2080" HTTPS_PROXY="http://127.0.0.1:2080" ALL_PROXY="socks5://127.0.0.1:2080"
export CGO_ENABLED=0
export GOOS=linux GOARCH=amd64
cd /home/jwk/code/nekoray
source libs/env_deploy.sh
mkdir -p deployment/linux64

# 构建updater/launcher
cd go/cmd/updater
go build -o ../../../deployment/linux64 -trimpath -ldflags "-w -s"
mv ../../../deployment/linux64/updater ../../../deployment/linux64/launcher

# 构建nekobox_core
cd ../nekobox_core
go build -v -o ../../../deployment/linux64 -trimpath -ldflags "-w -s -X github.com/matsuridayo/libneko/neko_common.Version_neko=$version_standalone" -tags "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_ech"
'
```

验证Go构建结果：
```bash
ls -la deployment/linux64/
# 应该看到: launcher 和 nekobox_core
```

### 4. 构建C++依赖

```bash
# 设置代理环境变量
export HTTP_PROXY="http://127.0.0.1:2080" HTTPS_PROXY="http://127.0.0.1:2080" ALL_PROXY="socks5://127.0.0.1:2080"

# 构建C++依赖（这一步可能需要较长时间）
./libs/build_deps_all.sh
```

### 5. 构建GUI部分

```bash
# 创建构建目录
mkdir -p build
cd build

# 配置cmake
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..

# 开始构建
ninja

# 部署
cd ..
./libs/deploy_linux64.sh
```

## 常见问题及解决方案

### 1. Go版本过低
**错误**: `package crypto/ecdh is not in GOROOT`
**解决**: 安装Go 1.22+版本

### 2. 缺少子模块
**错误**: `The source directory /home/jwk/code/nekoray/3rdparty/QHotkey does not contain a CMakeLists.txt file`
**解决**: 运行 `git submodule update --init --recursive`

### 3. 缺少Qt5X11Extras
**错误**: `Could not find a package configuration file provided by "Qt5X11Extras"`
**解决**: 安装 `sudo apt install libqt5x11extras5-dev`

### 4. 网络连接问题
**错误**: DNS解析失败或连接超时
**解决**: 设置代理环境变量

### 5. Shell兼容性问题
**错误**: fish shell语法错误
**解决**: 使用bash执行构建脚本

## 验证构建结果

构建完成后，检查最终输出：

```bash
ls -la deployment/linux64/
# 应该包含：
# - nekobox (GUI可执行文件)
# - nekobox_core (核心程序)
# - launcher (启动器)
# - 其他依赖文件
```

运行程序：
```bash
# 使用系统Qt运行库
./deployment/linux64/nekobox

# 或使用bundle版本（如果有launcher）
./deployment/linux64/launcher
```

## 参考文档

- [Build_Linux.md](./Build_Linux.md) - 官方Linux构建文档
- [Build_Core.md](./Build_Core.md) - Go核心构建文档
- [.github/workflows/build-nekoray-cmake.yml](../.github/workflows/build-nekoray-cmake.yml) - CI构建流程参考

## 构建环境说明

本指南基于以下环境测试：
- 操作系统：Ubuntu 22.04 LTS
- Go版本：1.22.5
- Qt版本：5.15.3
- CMake版本：3.22.1
- 构建工具：Ninja 1.10.1

构建成功后，你将获得一个完整的Nekoray应用程序，可以在Linux环境下运行。
