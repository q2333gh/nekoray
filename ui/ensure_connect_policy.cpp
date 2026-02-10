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

bool shouldRunNow(int failCount, long long nextRetryAtMs, long long nowMs) {
    if (!canRetry(failCount)) return false;
    return nextRetryAtMs <= 0 || nowMs >= nextRetryAtMs;
}

void markSuccess(int &failCount, long long &nextRetryAtMs) {
    failCount = 0;
    nextRetryAtMs = 0;
}

} // namespace EnsureConnectPolicy

