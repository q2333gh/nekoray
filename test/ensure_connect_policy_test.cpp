#include <QtTest/QtTest>

#include "ui/ensure_connect_policy.h"

class EnsureConnectPolicyTest : public QObject {
    Q_OBJECT

private slots:
    void backoff_doubles_each_failure();
    void stop_after_three_failures();
    void can_retry_by_timestamp();
    void reset_after_success();
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

void EnsureConnectPolicyTest::can_retry_by_timestamp() {
    const long long now = 10000;
    QVERIFY(EnsureConnectPolicy::shouldRunNow(0, 0, now));
    QVERIFY(!EnsureConnectPolicy::shouldRunNow(1, 20000, now));
    QVERIFY(EnsureConnectPolicy::shouldRunNow(1, 9000, now));
    QVERIFY(!EnsureConnectPolicy::shouldRunNow(3, 9000, now));
}

void EnsureConnectPolicyTest::reset_after_success() {
    int failCount = 2;
    long long nextRetryAtMs = 12345;
    EnsureConnectPolicy::markSuccess(failCount, nextRetryAtMs);
    QCOMPARE(failCount, 0);
    QCOMPARE(nextRetryAtMs, 0LL);
}

QTEST_MAIN(EnsureConnectPolicyTest)

#include "ensure_connect_policy_test.moc"
