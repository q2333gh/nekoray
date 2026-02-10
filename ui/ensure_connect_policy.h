#pragma once

namespace EnsureConnectPolicy {

int nextDelaySec(int baseIntervalSec, int failCount);
bool canRetry(int failCount);
bool shouldRunNow(int failCount, long long nextRetryAtMs, long long nowMs);
void markSuccess(int &failCount, long long &nextRetryAtMs);

} // namespace EnsureConnectPolicy

