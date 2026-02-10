# Nekoray 项目优化分析

## 1. 内存管理（高优先级）

项目大量使用裸指针 `new`/`delete`，缺乏 RAII 保护：

- 全局单例如 `profileManager`、`dataStore`、`trafficLooper` 均通过 `new` 分配，从未释放
- `sys/ExternalProcess.cpp` 中静态 `QFile *debugLogFile` 分配后未释放
- `db/traffic/TrafficLooper.cpp` 中手动 `new`/`delete` 配对容易出错

**建议：** 使用 `std::unique_ptr` / `std::shared_ptr` 替代裸指针

## 2. 不安全的类型转换（高优先级）

`main/NekoGui.cpp` 中大量 C 风格强制转换：

```cpp
*(QString *) item->ptr = *(QString *) p;
*(bool *) item->ptr = *(bool *) p;
```

**建议：** 使用 `static_cast` 或模板化方案替代，避免潜在的内存损坏

## 3. 线程安全问题

- `db/traffic/TrafficLooper.cpp` 中 `loop_enabled` 标志在无锁情况下被多线程访问
- `sys/ExternalProcess.cpp` 中 lambda 以 `[&]` 捕获信号处理器，存在 use-after-free 风险
- 静态文件句柄非线程安全

**建议：** 对所有共享状态使用 `std::atomic` 或适当的同步原语

## 4. 性能瓶颈

- **轮询式流量更新：** `TrafficLooper` 使用 `while(true)` + `QThread::msleep()` 轮询，应改为事件驱动（Qt 信号/槽）
- **重复字符串解析：** `fmt/Link2Bean.cpp` 中大量正则/字符串操作无缓存
- **UI 线程阻塞：** 文件 I/O 操作在 UI 线程中执行

## 5. 大函数/高认知复杂度

| 文件 | 行数 |
|------|------|
| `db/ConfigBuilder.cpp` | 839 |
| `ui/mainwindow.cpp` | 788 |
| `sub/GroupUpdater.cpp` | 708 |
| `ui/edit/dialog_edit_profile.cpp` | 566 |

**建议：** 拆分为更小的、可测试的函数单元

## 6. 未完成的 TODO/FIXME

- `ui/mainwindow_vpn.cpp`: `vpn_pid = 1; // TODO get pid?` — 硬编码 PID
- `fmt/Link2Bean.cpp`: `// TODO quic & kcp` — 缺失协议支持
- `db/ConfigBuilder.cpp`: `// TODO force enable?` — 未决策的逻辑

## 7. 安全隐患

- `fmt/Bean2External.cpp` 中命令参数拼接存在**命令注入风险**
- JSON 配置反序列化缺乏充分校验
- `db/Database.cpp` 中使用魔术数字 `-114514` 检测数据损坏，可读性差

## 8. 架构层面

- **全局单例过多**，导致模块间紧耦合，难以测试
- **UI 与业务逻辑耦合**，缺乏依赖注入
- **错误处理不一致** — 有的返回 `nullptr`，有的返回空字符串，无统一策略
- **缺少单元测试**

---

**总结：** 项目整体架构分层清晰（UI / DB / 格式转换 / 系统集成），但在内存安全、线程安全、代码复杂度方面有较大改进空间。建议优先处理**内存管理**和**线程安全**问题，这两项对稳定性影响最大。

## 9. UX 与新功能优化候选（新增）

下面给出 3 组可选方案，按“用户体感提升 + 落地成本 + 技术风险”综合排序。

### 选项 A：连接可观测性升级（推荐优先）

- 目标：让用户快速知道“当前是否可用、为什么不可用、下一步做什么”
- 可做内容：
- 在主界面新增连接状态卡片（运行状态、最近一次 URL Test、当前出口节点、错误摘要）
- 为 `Tun Mode`、`System Proxy`、`Ensure Connect` 增加状态提示与失败原因气泡
- 新增“最近 5 次失败记录”（时间、错误类型、恢复建议）
- 价值：降低排障成本，减少“看日志才知道问题”的路径
- 成本：中（主要是 UI 展示层 + 少量状态聚合逻辑）
- 风险：低（对核心链路侵入小）

### 选项 B：节点操作效率优化（推荐第二优先）

- 目标：提升节点筛选、对比、切换的效率
- 可做内容：
- 在 `proxyListTable` 增加快速过滤器（按协议、地区、延迟区间、可用性）
- 新增“收藏/置顶节点”和“一键切回上一个稳定节点”
- URL Test 结果增加排序与批量操作（批量测速、批量启用/禁用）
- 价值：高频操作更顺手，能显著减少点击与试错
- 成本：中到中高（涉及列表状态管理与交互联动）
- 风险：中（需要处理现有表格行为兼容）

### 选项 C：配置安全与新手引导（推荐第三优先）

- 目标：降低误操作与学习门槛
- 可做内容：
- 首次启动引导（导入订阅 -> 测速 -> 设为系统代理 -> 连通性验证）
- 危险操作二次确认（清空日志、重启程序、覆盖配置）
- 配置变更预检查（端口冲突、权限不足、无可用节点）并给出可执行修复建议
- 价值：减少“能用但不会用”的流失，新用户转化更好
- 成本：中（交互流程设计 + 校验规则）
- 风险：低到中（需避免过度打扰）

## 10. 建议选择方式

如果当前目标是“先提升留存和稳定感知”，建议按以下顺序：

1. A（连接可观测性升级）
2. B（节点操作效率优化）
3. C（配置安全与新手引导）

如果当前目标是“先做增长和新手转化”，可改为：

1. C（配置安全与新手引导）
2. A（连接可观测性升级）
3. B（节点操作效率优化）

## 11. 建议下一步（用于立项）

- 从 A/B/C 中先选 1 个作为本迭代主题
- 为该选项补一页实现方案（界面变更点、数据结构变更点、验收标准）
- 先做最小可用版本（MVP），避免一次性改动过大

## 12. Ensure Connect 迭代说明（2026-02-10）

- 重试策略：当检测失败时按指数退避重试，延时为 `base_interval * 2^fail_count`
- 终止条件：连续失败达到 `3` 次后停止自动重试
- 持久化键（`groups/nekobox.json`）：
- `ensure_connect_enabled`
- `ensure_connect_interval_sec`
- `ensure_connect_fail_count`
- `ensure_connect_next_retry_at_ms`
- `ensure_connect_last_failure_at_ms`
- 重启恢复：程序启动后会读取上述键值，若仍处于可重试窗口则按窗口继续；若已达到失败上限则保持停止自动重试
- 手动复位：用户将 `Ensure Connect` 切换为 Off 再 On，会清空失败计数与下一次重试时间
