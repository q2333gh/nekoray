# 文件大小优化报告

本报告基于 file-size-limit 规范生成，用于识别需要优化的文件。

## 规范说明

- **A类**：允许接近600行（450-600 OK），但超过600必须拆分
  - 单窗口/单页面 UI 实现
  - 核心进程控制器
  - 单平台系统代理设置器
  - 日志 Model / 解析适配器

- **B类**：建议300-450行
  - MainWindow.cpp / AppController.cpp
  - ConfigRepository.cpp
  - SystemIntegration 相关

- **C类**：必须拆分（≥450基本就拆）
  - 多主题混杂、巨型分发逻辑、跨层依赖、函数过长

---

## 🔴 必须拆分（超过600行）

### ui\mainwindow_grpc.cpp
- **行数**: 829
- **原因**: 文件超过600行限制（当前829行）
- **建议**: 必须立即拆分，不允许豁免

### db\ConfigBuilder.cpp
- **行数**: 680
- **原因**: 文件超过600行限制（当前680行）
- **建议**: 必须立即拆分，不允许豁免

### ui\mainwindow.cpp
- **行数**: 630
- **原因**: 文件超过600行限制（当前630行）
- **建议**: 必须立即拆分，不允许豁免

## 🟠 C类：建议拆分（≥450行）

### sub\GroupUpdater.cpp
- **行数**: 568
- **原因**: 文件568行，≥450行建议拆分
- **建议**: 建议按职责拆分，避免多主题混杂

### ui\edit\dialog_edit_profile.cpp
- **行数**: 470
- **原因**: 文件470行，≥450行建议拆分
- **建议**: 建议按职责拆分，避免多主题混杂

## 🟢 B类：建议优化（300-450行）

### main\NekoGui.cpp
- **行数**: 397
- **原因**: 文件397行，在300-450范围内
- **建议**: 建议保持在300-450行范围内

### ui\mainwindow_menu.cpp
- **行数**: 352
- **原因**: MainWindow相关文件，当前352行
- **建议**: 建议控制在300-450行，超过说明UI在做流程编排

### ui\dialog_basic_settings.cpp
- **行数**: 339
- **原因**: 文件339行，在300-450范围内
- **建议**: 建议保持在300-450行范围内

### db\Database.cpp
- **行数**: 331
- **原因**: 文件331行，在300-450范围内
- **建议**: 建议保持在300-450行范围内

---

## 统计信息

- 必须拆分（>600行）: 3 个文件
- C类（≥450行）: 2 个文件
- A类（450-600行）: 0 个文件
- B类（300-450行）: 4 个文件
- 正常（<300行）: 102 个文件
- 自动生成: 0 个文件
- **总计**: 111 个文件

---

## 所有文件列表（按行数降序）

| 文件路径 | 行数 | 分类 | 状态 |
|---------|------|------|------|
| ui\mainwindow_grpc.cpp | 829 | 必须拆分 (超过600行) | 🔴 必须拆分 |
| db\ConfigBuilder.cpp | 680 | 必须拆分 (超过600行) | 🔴 必须拆分 |
| ui\mainwindow.cpp | 630 | 必须拆分 (超过600行) | 🔴 必须拆分 |
| sub\GroupUpdater.cpp | 568 | C类 (建议拆分) | 🟠 建议拆分 |
| ui\edit\dialog_edit_profile.cpp | 470 | C类 (建议拆分) | 🟠 建议拆分 |
| main\NekoGui.cpp | 397 | B类 (建议优化) | 🟢 建议优化 |
| ui\mainwindow_menu.cpp | 352 | B类 (MainWindow) | 🟢 建议优化 |
| ui\dialog_basic_settings.cpp | 339 | B类 (建议优化) | 🟢 建议优化 |
| db\Database.cpp | 331 | B类 (建议优化) | 🟢 建议优化 |
| ui\mainwindow_proxy_list.cpp | 276 | 正常 | ✅ 正常 |
| fmt\Link2Bean.cpp | 239 | 正常 | ✅ 正常 |
| rpc\gRPC.cpp | 239 | 正常 | ✅ 正常 |
| main\NekoGui_Utils.cpp | 222 | 正常 | ✅ 正常 |
| ui\dialog_manage_routes.cpp | 217 | 正常 | ✅ 正常 |
| fmt\Bean2External.cpp | 210 | 正常 | ✅ 正常 |
| sys\ExternalProcess.cpp | 210 | 正常 | ✅ 正常 |
| fmt\Bean2CoreObj_box.cpp | 202 | 正常 | ✅ 正常 |
| fmt\Bean2Link.cpp | 193 | 正常 | ✅ 正常 |
| main\main.cpp | 193 | 正常 | ✅ 正常 |
| ui\mainwindow_menubuilder.cpp | 170 | 正常 | ✅ 正常 |
| ui\mainwindow_vpn.cpp | 165 | 正常 | ✅ 正常 |
| sys\AutoRun.cpp | 158 | 正常 | ✅ 正常 |
| ui\mainwindow_log.cpp | 131 | 正常 | ✅ 正常 |
| ui\mainwindow_menubuilder.h | 130 | 正常 | ✅ 正常 |
| main\NekoGui_DataStore.hpp | 128 | 正常 | ✅ 正常 |
| ui\edit\edit_custom.cpp | 128 | 正常 | ✅ 正常 |
| ui\mainwindow.h | 126 | 正常 | ✅ 正常 |
| ui\edit\edit_quic.cpp | 113 | 正常 | ✅ 正常 |
| ui\widget\GroupItem.cpp | 107 | 正常 | ✅ 正常 |
| db\traffic\TrafficLooper.cpp | 105 | 正常 | ✅ 正常 |
| main\NekoGui_Utils.hpp | 97 | 正常 | ✅ 正常 |
| ui\edit\dialog_edit_group.cpp | 90 | 正常 | ✅ 正常 |
| fmt\QUICBean.hpp | 84 | 正常 | ✅ 正常 |
| ui\mainwindow_dialog.cpp | 81 | 正常 | ✅ 正常 |
| ui\widget\MyTableWidget.h | 81 | 正常 | ✅ 正常 |
| ui\mainwindow_status.cpp | 79 | 正常 | ✅ 正常 |
| main\HTTPRequestHelper.cpp | 73 | 正常 | ✅ 正常 |
| main\GuiUtils.hpp | 70 | 正常 | ✅ 正常 |
| ui\ThemeManager.cpp | 70 | 正常 | ✅ 正常 |
| ui\dialog_vpn_settings.cpp | 69 | 正常 | ✅ 正常 |
| fmt\AbstractBean.cpp | 67 | 正常 | ✅ 正常 |
| db\ProfileFilter.cpp | 65 | 正常 | ✅ 正常 |
| sys\windows\MiniDump.cpp | 64 | 正常 | ✅ 正常 |
| ui\edit\edit_chain.cpp | 64 | 正常 | ✅ 正常 |
| ui\edit\edit_naive.cpp | 56 | 正常 | ✅ 正常 |
| fmt\V2RayStreamSettings.hpp | 54 | 正常 | ✅ 正常 |
| db\ProxyEntity.hpp | 53 | 正常 | ✅ 正常 |
| ui\mainwindow_connection.cpp | 51 | 正常 | ✅ 正常 |
| ui\mainwindow_hotkey.cpp | 51 | 正常 | ✅ 正常 |
| ui\edit\dialog_edit_profile.h | 49 | 正常 | ✅ 正常 |
| main\NekoGui_ConfigItem.hpp | 48 | 正常 | ✅ 正常 |
| ui\dialog_manage_groups.cpp | 48 | 正常 | ✅ 正常 |
| db\ConfigBuilder.hpp | 46 | 正常 | ✅ 正常 |
| sub\GroupUpdater.hpp | 46 | 正常 | ✅ 正常 |
| fmt\CustomBean.hpp | 43 | 正常 | ✅ 正常 |
| fmt\AbstractBean.hpp | 41 | 正常 | ✅ 正常 |
| sys\linux\LinuxCap.cpp | 40 | 正常 | ✅ 正常 |
| ui\dialog_manage_routes.h | 40 | 正常 | ✅ 正常 |
| ui\edit\edit_socks_http.cpp | 38 | 正常 | ✅ 正常 |
| ui\Icon.cpp | 38 | 正常 | ✅ 正常 |
| ui\widget\ProxyItem.cpp | 38 | 正常 | ✅ 正常 |
| sys\ExternalProcess.hpp | 37 | 正常 | ✅ 正常 |
| ui\edit\edit_shadowsocks.cpp | 35 | 正常 | ✅ 正常 |
| db\Database.hpp | 33 | 正常 | ✅ 正常 |
| db\traffic\TrafficData.hpp | 32 | 正常 | ✅ 正常 |
| fmt\NaiveBean.hpp | 31 | 正常 | ✅ 正常 |
| db\ProfileFilter.hpp | 30 | 正常 | ✅ 正常 |
| ui\edit\edit_trojan_vless.cpp | 30 | 正常 | ✅ 正常 |
| ui\dialog_basic_settings.h | 28 | 正常 | ✅ 正常 |
| ui\widget\FloatCheckBox.h | 28 | 正常 | ✅ 正常 |
| ui\widget\MessageBoxTimer.h | 28 | 正常 | ✅ 正常 |
| rpc\gRPC.h | 27 | 正常 | ✅ 正常 |
| ui\edit\edit_naive.h | 27 | 正常 | ✅ 正常 |
| ui\edit\edit_quic.h | 27 | 正常 | ✅ 正常 |
| ui\widget\GroupItem.h | 27 | 正常 | ✅ 正常 |
| fmt\SocksHttpBean.hpp | 26 | 正常 | ✅ 正常 |
| ui\edit\edit_vmess.cpp | 25 | 正常 | ✅ 正常 |
| ui\widget\ProxyItem.h | 25 | 正常 | ✅ 正常 |
| fmt\ShadowSocksBean.hpp | 24 | 正常 | ✅ 正常 |
| fmt\TrojanVLESSBean.hpp | 24 | 正常 | ✅ 正常 |
| ui\edit\dialog_edit_group.h | 24 | 正常 | ✅ 正常 |
| ui\edit\edit_chain.h | 24 | 正常 | ✅ 正常 |
| ui\edit\edit_custom.h | 24 | 正常 | ✅ 正常 |
| main\HTTPRequestHelper.hpp | 23 | 正常 | ✅ 正常 |
| db\Group.hpp | 22 | 正常 | ✅ 正常 |
| db\traffic\TrafficLooper.hpp | 22 | 正常 | ✅ 正常 |
| fmt\VMessBean.hpp | 22 | 正常 | ✅ 正常 |
| main\Const.hpp | 22 | 正常 | ✅ 正常 |
| ui\dialog_hotkey.cpp | 22 | 正常 | ✅ 正常 |
| ui\dialog_manage_groups.h | 22 | 正常 | ✅ 正常 |
| ui\dialog_vpn_settings.h | 20 | 正常 | ✅ 正常 |
| ui\edit\edit_shadowsocks.h | 19 | 正常 | ✅ 正常 |
| ui\edit\edit_trojan_vless.h | 19 | 正常 | ✅ 正常 |
| ui\edit\edit_vmess.h | 19 | 正常 | ✅ 正常 |
| sys\windows\guihelper.cpp | 18 | 正常 | ✅ 正常 |
| ui\edit\edit_socks_http.h | 17 | 正常 | ✅ 正常 |
| ui\GroupSort.hpp | 17 | 正常 | ✅ 正常 |
| fmt\Preset.hpp | 16 | 正常 | ✅ 正常 |
| ui\dialog_hotkey.h | 16 | 正常 | ✅ 正常 |
| ui\edit\profile_editor.h | 15 | 正常 | ✅ 正常 |
| fmt\ChainBean.hpp | 13 | 正常 | ✅ 正常 |
| main\NekoGui.hpp | 13 | 正常 | ✅ 正常 |
| ui\Icon.hpp | 12 | 正常 | ✅ 正常 |
| ui\widget\MyLineEdit.h | 11 | 正常 | ✅ 正常 |
| fmt\includes.h | 9 | 正常 | ✅ 正常 |
| ui\ThemeManager.hpp | 8 | 正常 | ✅ 正常 |
| sys\linux\LinuxCap.h | 6 | 正常 | ✅ 正常 |
| sys\windows\guihelper.h | 4 | 正常 | ✅ 正常 |
| sys\AutoRun.hpp | 3 | 正常 | ✅ 正常 |
| ui\mainwindow_interface.h | 3 | 正常 | ✅ 正常 |
| sys\windows\MiniDump.h | 2 | 正常 | ✅ 正常 |