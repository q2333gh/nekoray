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
