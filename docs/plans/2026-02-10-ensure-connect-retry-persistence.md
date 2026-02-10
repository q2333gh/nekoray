# Ensure Connect Exponential Backoff + Persistence Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 在 Ensure Connect 检测失败时按 2x 退避重试，失败达到 3 次后停止自动重试，并把状态持久化到 `groups/nekobox.json`，保证重启后可继续同一决策。

**Architecture:** 将“重试策略”抽到纯逻辑模块（可单测），UI 层只负责采样结果与驱动状态机。持久化统一走 `NekoGui::DataStore`，在状态变化点立即 `Save()`，重启后恢复策略状态并决定是否继续重试。

**Tech Stack:** C++17, Qt (QTimer/QDateTime/QJson), existing JsonStore(DataStore), CMake

---

## 方案选型（brainstorming）

1. 方案 A（推荐）：`DataStore` 持久化 + 纯逻辑策略函数
- 优点：可测试、风险低、改动边界清晰。
- 缺点：需要新增少量字段与 helper 文件。

2. 方案 B：仅在 `mainwindow_ensure_connect.cpp` 内联实现全部逻辑
- 优点：改文件少。
- 缺点：难测试，后续维护成本高。

3. 方案 C：单独新建 `ensure_connect_state.json`
- 优点：业务状态隔离。
- 缺点：引入新文件生命周期与兼容逻辑，不如复用 DataStore 简单。

本计划按方案 A 执行。

## 需求约束（本计划默认）

- 基础周期取 `spinBox_ensure_connect_interval`（秒）。
- 每次失败后下次重试延时翻倍：`delay = base * 2^fail_count`（第一次失败后等待 `base*2`）。
- `fail_count >= 3` 时停止自动重试（需要用户手动重置，例如切换按钮 Off/On）。
- 持久化字段至少包括：`enabled/base_interval/fail_count/next_retry_at_ms/last_failure_ts_ms`。
- 重启后若 `enabled=true` 且 `fail_count<3` 且当前时间已到 `next_retry_at_ms`，允许重试；否则继续等待或保持停止。

### Task 1: 提取策略逻辑并先写失败测试

**Files:**
- Create: `ui/ensure_connect_policy.h`
- Create: `ui/ensure_connect_policy.cpp`
- Create: `test/ensure_connect_policy_test.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Write the failing test**

```cpp
// test/ensure_connect_policy_test.cpp
#include <QtTest/QtTest>
#include "ui/ensure_connect_policy.h"

class EnsureConnectPolicyTest : public QObject {
    Q_OBJECT
private slots:
    void backoff_doubles_each_failure();
    void stop_after_three_failures();
};

void EnsureConnectPolicyTest::backoff_doubles_each_failure() {
    const int baseSec = 60;
    QCOMPARE(EnsureConnectPolicy::nextDelaySec(baseSec, 1), 120);
    QCOMPARE(EnsureConnectPolicy::nextDelaySec(baseSec, 2), 240);
    QCOMPARE(EnsureConnectPolicy::nextDelaySec(baseSec, 3), 480);
}

void EnsureConnectPolicyTest::stop_after_three_failures() {
    QVERIFY(EnsureConnectPolicy::canRetry(0));
    QVERIFY(EnsureConnectPolicy::canRetry(2));
    QVERIFY(!EnsureConnectPolicy::canRetry(3));
}

QTEST_MAIN(EnsureConnectPolicyTest)
#include "ensure_connect_policy_test.moc"
```

**Step 2: Run test to verify it fails**

Run:
```bash
cmake -S . -B build -DQT_VERSION_MAJOR=6 -DNEKO_BUILD_TESTS=ON
cmake --build build --target ensure_connect_policy_test
ctest --test-dir build -R ensure_connect_policy_test --output-on-failure
```

Expected: FAIL（缺少 `EnsureConnectPolicy` 头/实现或符号未定义）

**Step 3: Write minimal implementation**

```cpp
// ui/ensure_connect_policy.h
#pragma once

namespace EnsureConnectPolicy {
int nextDelaySec(int baseIntervalSec, int failCount);
bool canRetry(int failCount);
}
```

```cpp
// ui/ensure_connect_policy.cpp
#include "ensure_connect_policy.h"

#include <algorithm>

namespace EnsureConnectPolicy {
int nextDelaySec(int baseIntervalSec, int failCount) {
    const int base = std::max(1, baseIntervalSec);
    const int exp = std::max(0, failCount);
    int delay = base;
    for (int i = 0; i < exp; ++i) {
        delay *= 2;
    }
    return delay;
}

bool canRetry(int failCount) {
    return failCount < 3;
}
}
```

**Step 4: Run test to verify it passes**

Run:
```bash
cmake --build build --target ensure_connect_policy_test
ctest --test-dir build -R ensure_connect_policy_test --output-on-failure
```

Expected: PASS

**Step 5: Commit**

```bash
git add CMakeLists.txt ui/ensure_connect_policy.h ui/ensure_connect_policy.cpp test/ensure_connect_policy_test.cpp
git commit -m "test+feat: add ensure connect retry policy module"
```

### Task 2: 增加 DataStore 持久化字段

**Files:**
- Modify: `main/NekoGui_DataStore.hpp`
- Modify: `main/NekoGui.cpp`

**Step 1: Write the failing test**

```cpp
// test/ensure_connect_policy_test.cpp (新增用例)
void EnsureConnectPolicyTest::can_retry_by_timestamp() {
    const long long now = 10000;
    QVERIFY(EnsureConnectPolicy::shouldRunNow(0, 0, now));
    QVERIFY(!EnsureConnectPolicy::shouldRunNow(1, 20000, now));
    QVERIFY(EnsureConnectPolicy::shouldRunNow(1, 9000, now));
    QVERIFY(!EnsureConnectPolicy::shouldRunNow(3, 9000, now));
}
```

**Step 2: Run test to verify it fails**

Run:
```bash
cmake --build build --target ensure_connect_policy_test
ctest --test-dir build -R ensure_connect_policy_test --output-on-failure
```

Expected: FAIL（`shouldRunNow` 未定义）

**Step 3: Write minimal implementation + datastore key mapping**

```cpp
// main/NekoGui_DataStore.hpp (新增字段)
bool ensure_connect_enabled = false;
int ensure_connect_interval_sec = 60;
int ensure_connect_fail_count_saved = 0;
long long ensure_connect_next_retry_at_ms = 0;
long long ensure_connect_last_failure_at_ms = 0;
```

```cpp
// main/NekoGui.cpp::DataStore::DataStore() (新增 _add)
_add(new configItem("ensure_connect_enabled", &ensure_connect_enabled, itemType::boolean));
_add(new configItem("ensure_connect_interval_sec", &ensure_connect_interval_sec, itemType::integer));
_add(new configItem("ensure_connect_fail_count", &ensure_connect_fail_count_saved, itemType::integer));
_add(new configItem("ensure_connect_next_retry_at_ms", &ensure_connect_next_retry_at_ms, itemType::integer64));
_add(new configItem("ensure_connect_last_failure_at_ms", &ensure_connect_last_failure_at_ms, itemType::integer64));
```

```cpp
// ui/ensure_connect_policy.h (新增)
bool shouldRunNow(int failCount, long long nextRetryAtMs, long long nowMs);
```

```cpp
// ui/ensure_connect_policy.cpp (新增)
bool shouldRunNow(int failCount, long long nextRetryAtMs, long long nowMs) {
    if (!canRetry(failCount)) return false;
    return nextRetryAtMs <= 0 || nowMs >= nextRetryAtMs;
}
```

**Step 4: Run test to verify it passes**

Run:
```bash
cmake --build build --target ensure_connect_policy_test
ctest --test-dir build -R ensure_connect_policy_test --output-on-failure
```

Expected: PASS

**Step 5: Commit**

```bash
git add main/NekoGui_DataStore.hpp main/NekoGui.cpp ui/ensure_connect_policy.h ui/ensure_connect_policy.cpp test/ensure_connect_policy_test.cpp
git commit -m "feat: persist ensure connect retry state in datastore"
```

### Task 3: 接入 Ensure Connect 运行时状态机

**Files:**
- Modify: `ui/mainwindow.h`
- Modify: `ui/mainwindow_ensure_connect.cpp`

**Step 1: Write the failing test**

```cpp
// test/ensure_connect_policy_test.cpp (新增)
void EnsureConnectPolicyTest::reset_after_success() {
    int failCount = 2;
    long long nextRetryAtMs = 12345;
    EnsureConnectPolicy::markSuccess(failCount, nextRetryAtMs);
    QCOMPARE(failCount, 0);
    QCOMPARE(nextRetryAtMs, 0LL);
}
```

**Step 2: Run test to verify it fails**

Run:
```bash
cmake --build build --target ensure_connect_policy_test
ctest --test-dir build -R ensure_connect_policy_test --output-on-failure
```

Expected: FAIL（`markSuccess` 未定义）

**Step 3: Write minimal implementation + UI integration**

```cpp
// ui/ensure_connect_policy.h (新增)
void markSuccess(int &failCount, long long &nextRetryAtMs);
```

```cpp
// ui/ensure_connect_policy.cpp (新增)
void markSuccess(int &failCount, long long &nextRetryAtMs) {
    failCount = 0;
    nextRetryAtMs = 0;
}
```

```cpp
// ui/mainwindow_ensure_connect.cpp (关键接入点，示意)
// 1) setup 时从 dataStore 恢复按钮状态/间隔/失败计数/next_retry
// 2) timer timeout 前先判断 shouldRunNow(..., QDateTime::currentMSecsSinceEpoch())
// 3) eval 失败时 fail_count++，计算 next_retry_at_ms = now + nextDelaySec(...)*1000
// 4) 成功时 markSuccess(...)
// 5) 每次状态变化后同步写回 dataStore 并 Save()
```

**Step 4: Run test + basic build**

Run:
```bash
cmake --build build --target nekobox
cmake --build build --target ensure_connect_policy_test
ctest --test-dir build -R ensure_connect_policy_test --output-on-failure
```

Expected: `nekobox` build success + tests PASS

**Step 5: Commit**

```bash
git add ui/mainwindow.h ui/mainwindow_ensure_connect.cpp ui/ensure_connect_policy.h ui/ensure_connect_policy.cpp test/ensure_connect_policy_test.cpp
git commit -m "feat: add exponential backoff and stop-after-3 for ensure connect"
```

### Task 4: 持久化重启行为与手工验收

**Files:**
- Modify: `docs/optimization_analysis.md`
- (Optional) Modify: `ui/mainwindow.ui`（如需显示“已停止重试”状态文案）

**Step 1: Write the failing manual test checklist**

```text
场景 A: 失败 1 次 -> 下次等待 base*2
场景 B: 连续失败到第 3 次 -> 自动停止，不再触发重试
场景 C: 重启应用后 -> 保持 fail_count/next_retry_at_ms，不提前重试
场景 D: 用户手动 Off/On -> 重置 fail_count 与 next_retry_at_ms
```

**Step 2: Run manual verification (first expected fail before full wiring)**

Run:
```bash
cmake --build build --target nekobox
```

Expected: 编译成功，但若逻辑未全接入会出现场景 C/D 不满足

**Step 3: 完成剩余接线并补文档**

```markdown
- 记录持久化键含义
- 记录“超过 3 次停止重试”的触发条件
- 记录如何手动复位（Off/On）
```

**Step 4: Re-run verification**

Run:
```bash
cmake --build build --target nekobox
```

Expected: 构建成功，场景 A/B/C/D 全部符合

**Step 5: Commit**

```bash
git add docs/optimization_analysis.md ui/mainwindow.ui
git commit -m "docs: document ensure connect retry persistence and reset behavior"
```

## 验收标准（Definition of Done）

- 失败退避为 2x 指数增长。
- 连续失败达到 3 次后，不再自动重试。
- `groups/nekobox.json` 能看到并恢复重试状态键值。
- 重启后行为与重启前一致（不会丢状态、不提前重试）。
- 用户手动 Off/On 后可重新开始重试链路。
