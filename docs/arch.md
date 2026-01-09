 

## Nekoray 架构层次分析

### 整体架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                    应用入口层 (Entry Layer)                  │
│  main/main.cpp - 程序入口、初始化、单例管理                  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                    UI 展示层 (Presentation Layer)            │
│  ui/mainwindow.* - 主窗口、用户交互                          │
│  ui/dialog_* - 各种设置对话框                                │
│  ui/edit/* - 代理配置编辑界面                                │
│  ui/widget/* - UI 组件                                      │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   业务逻辑层 (Business Logic Layer)          │
│  sub/GroupUpdater - 订阅更新管理                             │
│  db/ConfigBuilder - 配置构建器                               │
│  db/ProfileFilter - 配置文件过滤                             │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   数据模型层 (Data Model Layer)               │
│  db/Database - 数据存储管理                                  │
│  db/ProfileManager - 配置文件和组管理                        │
│  db/ProxyEntity - 代理实体                                   │
│  db/Group - 组管理                                           │
│  main/NekoGui_DataStore - 全局数据存储                       │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   格式转换层 (Format Layer)                   │
│  fmt/AbstractBean - 抽象 Bean 基类                           │
│  fmt/*Bean - 各种协议 Bean (VMess, VLESS, Trojan, etc.)     │
│  fmt/Bean2CoreObj - Bean 转核心配置                          │
│  fmt/Bean2Link - Bean 转分享链接                            │
│  fmt/Link2Bean - 分享链接转 Bean                             │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   系统交互层 (System Layer)                  │
│  sys/ExternalProcess - 外部进程管理                          │
│  sys/CoreProcess - 核心进程管理                              │
│  sys/AutoRun - 自启动管理                                    │
│  rpc/gRPC - gRPC 通信                                        │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                   核心进程层 (Core Process Layer)             │
│  go/cmd/nekobox_core - Go 语言核心进程                        │
│  go/grpc_server - gRPC 服务器                                │
│  (sing-box 核心)                                             │
└─────────────────────────────────────────────────────────────┘
```

### 详细层次说明

#### 1. 应用入口层 (`main/`)
- `main.cpp`: 程序入口
  - 初始化 Qt 应用
  - 单例检查 (RunGuard)
  - 加载配置和数据存储
  - 初始化主窗口
- `NekoGui.hpp`: 核心命名空间和常量定义
- `NekoGui_DataStore.hpp`: 全局数据存储（配置、状态、标志）

#### 2. UI 展示层 (`ui/`)
- `mainwindow.*`: 主窗口
  - 代理列表展示
  - 启动/停止代理
  - 系统代理/VPN 模式切换
  - 日志显示
- `dialog_*`: 设置对话框
  - `dialog_basic_settings`: 基础设置
  - `dialog_manage_groups`: 组管理
  - `dialog_manage_routes`: 路由管理
  - `dialog_vpn_settings`: VPN 设置
  - `dialog_hotkey`: 快捷键设置
- `edit/*`: 代理编辑界面
  - 各协议的编辑界面
  - 链式代理编辑

#### 3. 业务逻辑层 (`sub/`, `db/`)
- `sub/GroupUpdater`: 订阅更新
  - 解析多种订阅格式（Clash、Raw、Base64）
  - 协议解析（VMess、VLESS、Trojan 等）
  - 订阅更新逻辑
- `db/ConfigBuilder`: 配置构建
  - 将 `ProxyEntity` 转换为核心配置
  - 路由规则构建
  - VPN 配置生成
- `db/ProfileFilter`: 配置文件过滤和比较

#### 4. 数据模型层 (`db/`, `main/`)
- `db/Database.*`: 数据持久化
  - JSON 存储抽象 (`JsonStore`)
  - 配置文件的加载和保存
- `db/ProfileManager`: 配置文件和组管理
  - 代理实体管理
  - 组管理
  - ID 分配和排序
- `db/ProxyEntity`: 代理实体
  - 包含 `AbstractBean`（协议配置）
  - 延迟、流量数据
- `db/Group`: 组实体
  - 订阅 URL
  - 代理列表和顺序
- `main/NekoGui_DataStore`: 全局状态
  - 运行状态
  - 用户设置
  - 路由配置

#### 5. 格式转换层 (`fmt/`)
- `fmt/AbstractBean.hpp`: 抽象基类
  - 所有协议 Bean 的基类
  - 提供 `BuildCoreObjSingBox()` 等接口
- 具体协议 Bean:
  - `VMessBean`: VMess 协议
  - `TrojanVLESSBean`: Trojan/VLESS 协议
  - `ShadowSocksBean`: Shadowsocks 协议
  - `SocksHttpBean`: SOCKS/HTTP 协议
  - `QUICBean`: QUIC 协议（Hysteria2、TUIC）
  - `NaiveBean`: NaiveProxy 协议
  - `CustomBean`: 自定义配置
  - `ChainBean`: 链式代理
- `fmt/Bean2CoreObj_box.cpp`: Bean → 核心配置 JSON
- `fmt/Bean2Link.cpp`: Bean → 分享链接
- `fmt/Link2Bean.cpp`: 分享链接 → Bean

#### 6. 系统交互层 (`sys/`, `rpc/`)
- `sys/ExternalProcess`: 外部进程管理基类
  - 进程启动/停止
  - 输出捕获
- `sys/CoreProcess`: 核心进程管理
  - 启动 `nekobox_core`
  - 进程状态监控
  - 自动重启
- `sys/AutoRun`: 系统自启动管理
- `rpc/gRPC.*`: gRPC 客户端
  - 与核心进程通信
  - 配置加载/停止
  - 连接查询
  - 统计查询

#### 7. 核心进程层 (`go/`)
- `go/cmd/nekobox_core/`: Go 语言核心进程
  - 基于 sing-box
  - 提供 gRPC 服务
- `go/grpc_server/`: gRPC 服务器实现
  - 处理来自 GUI 的请求
  - 管理 sing-box 实例

### 数据流向

```
用户操作 (UI)
    ↓
MainWindow 处理事件
    ↓
业务逻辑层 (GroupUpdater, ConfigBuilder)
    ↓
数据模型层 (ProfileManager, ProxyEntity)
    ↓
格式转换层 (Bean → JSON)
    ↓
系统交互层 (gRPC Client)
    ↓
核心进程 (gRPC Server + sing-box)
```

### 关键设计模式

1. 单例模式: `dataStore`, `profileManager`
2. 工厂模式: `ProfileManager::NewProxyEntity()` 创建不同协议实体
3. 策略模式: 各协议 Bean 实现 `BuildCoreObjSingBox()`
4. 观察者模式: Qt 信号槽机制
5. 适配器模式: `Bean2CoreObj` 将内部格式转换为核心配置

### 存储结构

```
config/
├── groups/              # 组配置文件
│   └── *.json
├── profiles/            # 代理配置文件
│   └── *.json
├── routes_box/         # 路由配置
│   └── *.json
└── groups/nekobox.json # 主配置文件（包含组顺序）
```

### 通信机制

- GUI ↔ Core: gRPC (HTTP/2)
- 核心进程: Go 语言实现，基于 sing-box
- 配置格式: JSON（内部格式和 sing-box 配置格式）

该架构采用分层设计，职责清晰，便于维护和扩展。