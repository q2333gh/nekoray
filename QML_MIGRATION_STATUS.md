# QML 迁移状态文档

## 迁移概述

本项目已完成从 Qt Widgets 到 Qt 6 QML 的架构重构。当前状态为**混合模式**：QML UI 已实现并可用，但保留了部分 Widgets 代码用于过渡。

## 已完成的工作

### 1. 构建系统 ✅
- CMakeLists.txt 升级到 Qt 6
- 添加 Quick、QuickControls2 模块
- 移除 Widgets 依赖（保留用于过渡）

### 2. 数据模型层 ✅
- `ProfileModel` (QAbstractListModel) - 完整的代理列表模型
- `GroupModel` (QAbstractListModel) - 组列表模型
- 所有角色和属性正确暴露给 QML

### 3. 控制器层 ✅
- `MainWindowController` - 主窗口业务逻辑
- `BasicSettingsController` - 基础设置控制器
- `EditProfileController` - 编辑配置文件控制器
- `MainWindowAdapter` - 桥接现有 MainWindow 代码

### 4. QML UI 组件 ✅
- **主窗口** (`qml/main.qml`) - 完整的 ApplicationWindow
- **代理列表** (`qml/components/ProxyItem.qml`) - 支持拖拽和上下文菜单
- **基础设置对话框** (`qml/dialogs/BasicSettingsDialog.qml`) - 完整实现
- **编辑配置文件对话框** (`qml/edit/EditProfileDialog.qml`) - 框架完成
- **协议编辑界面** - 所有协议的编辑界面已创建（VMess, Trojan/VLESS, Shadowsocks, SOCKS/HTTP, Naive, QUIC, Chain）
- **系统托盘** - 使用 Qt.labs.platform.SystemTrayIcon
- **快捷键** - Shortcut 组件集成

### 5. 主程序入口 ✅
- main.cpp 完全迁移到 QML
- QQmlApplicationEngine 集成
- 类型注册和上下文属性设置

## 当前架构

```
QML UI Layer (qml/)
├── main.qml (主窗口)
├── components/
│   ├── ProxyItem.qml ✅
│   ├── GroupItem.qml (占位符)
│   └── ProxyTable.qml (占位符)
├── dialogs/
│   ├── BasicSettingsDialog.qml ✅
│   ├── HotkeyDialog.qml (占位符)
│   ├── ManageGroupsDialog.qml (占位符)
│   ├── ManageRoutesDialog.qml (占位符)
│   └── VPNSettingsDialog.qml (占位符)
├── edit/
│   ├── EditProfileDialog.qml ✅
│   ├── EditVMess.qml ✅
│   ├── EditTrojanVLESS.qml ✅
│   ├── EditShadowSocks.qml ✅
│   ├── EditSocksHttp.qml ✅
│   ├── EditNaive.qml ✅
│   ├── EditQUIC.qml ✅
│   └── EditChain.qml ✅
└── pages/
    ├── ProxyListPage.qml (占位符)
    ├── LogPage.qml (占位符)
    └── ConnectionPage.qml (占位符)

C++ Backend (ui/qml/)
├── ProfileModel.* ✅
├── GroupModel.* ✅
├── MainWindowController.* ✅
├── BasicSettingsController.* ✅
├── EditProfileController.* ✅
└── MainWindowAdapter.* ✅
```

## 过渡期说明

### 当前混合模式

项目当前运行在**混合模式**下：
- QML UI 作为主界面显示
- MainWindow Widget 被隐藏但保留，用于：
  - 核心业务逻辑（neko_start, neko_stop 等）
  - gRPC 通信
  - 系统集成功能

### 为什么保留 MainWindow Widget？

1. **核心逻辑复杂**：MainWindow 包含大量业务逻辑，需要逐步迁移
2. **系统集成**：某些系统功能（如 VPN、系统代理）需要 Widgets 支持
3. **稳定性**：保留现有代码确保功能可用，逐步迁移降低风险

## 待完善的功能

### 高优先级
1. **其他对话框**
   - HotkeyDialog - 快捷键设置
   - ManageGroupsDialog - 组管理
   - ManageRoutesDialog - 路由管理
   - VPNSettingsDialog - VPN 设置

2. **编辑界面完善**
   - 各协议编辑界面的完整实现
   - 表单验证
   - 数据保存逻辑

3. **功能实现**
   - 从剪贴板添加配置
   - 克隆配置
   - 移动配置到其他组

### 中优先级
1. **UI 优化**
   - 拖拽排序的完整实现
   - 列表性能优化
   - 动画和过渡效果

2. **主题系统**
   - QML 主题系统完善
   - 深色模式支持

### 低优先级（清理阶段）
1. **完全移除 Widgets**
   - 迁移所有核心逻辑到 Controller
   - 移除 MainWindow Widget
   - 删除所有 .ui 文件
   - 清理 Widgets 相关代码

## 编译和运行

### 当前状态
- ✅ 可以编译（需要 Qt 6）
- ✅ QML UI 可以显示
- ✅ 基本功能可用（启动/停止、列表显示）
- ⚠️ 部分功能需要完善

### 编译要求
- Qt 6.5 或更高版本
- CMake 3.16+
- C++17 编译器

### 运行
```bash
# Windows
cd build/Release
.\nekobox.exe

# Linux
./nekobox
```

## 迁移路径

### 阶段 1: 当前状态（混合模式）✅
- QML UI 显示
- Widgets 代码保留用于核心逻辑
- 功能基本可用

### 阶段 2: 功能完善（进行中）
- 实现所有对话框
- 完善编辑界面
- 实现所有业务功能

### 阶段 3: 完全迁移（未来）
- 将所有核心逻辑迁移到 Controller
- 移除 MainWindow Widget
- 删除所有 .ui 文件
- 完全移除 Widgets 依赖

## 文件清单

### 新增文件
- `qml/` - 所有 QML UI 文件
- `ui/qml/` - QML 后端模型和控制器

### 保留文件（过渡期）
- `ui/mainwindow.*` - 保留用于核心逻辑
- `ui/*.ui` - 保留但不再使用
- `ui/widget/*` - 保留但不再使用

### 待删除文件（阶段 3）
- 所有 `.ui` 文件
- `ui_mainwindow.h` 等生成的 UI 头文件
- Widgets 相关实现文件

## 注意事项

1. **不要删除 .ui 文件**：当前仍需要它们来编译（过渡期）
2. **MainWindow Widget 被隐藏**：在 main.cpp 中调用 `hide()`
3. **功能测试**：确保所有功能在 QML UI 中正常工作
4. **性能监控**：QML 列表性能需要验证和优化

## 下一步行动

1. 完善所有对话框功能
2. 完善编辑界面功能
3. 实现所有业务逻辑
4. 测试和调试
5. 性能优化
6. 完全移除 Widgets（阶段 3）
