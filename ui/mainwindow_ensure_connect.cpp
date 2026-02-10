#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include "mainwindow_menubuilder.h"

#include "db/Database.hpp"
#include "ensure_connect_policy.h"

#include <QDateTime>
#include <QSpinBox>
#include <QTimer>
#include <algorithm>

void MainWindow::setup_ensure_connect_controls() {
    ensure_connect_timer = new QTimer(this);
    ensure_connect_timer->setSingleShot(false);
    ensure_connect_eval_timer = new QTimer(this);
    ensure_connect_eval_timer->setSingleShot(true);

    auto persist_ensure_connect_state = [=]() {
        NekoGui::dataStore->ensure_connect_enabled = ui->toolButton_ensure_connect->isChecked();
        NekoGui::dataStore->ensure_connect_interval_sec = ui->spinBox_ensure_connect_interval->value();
        NekoGui::dataStore->ensure_connect_fail_count = ensure_connect_fail_count;
        NekoGui::dataStore->ensure_connect_next_retry_at_ms = ensure_connect_next_retry_at_ms;
        NekoGui::dataStore->ensure_connect_last_failure_at_ms = ensure_connect_last_failure_at_ms;
        NekoGui::dataStore->Save();
    };

    auto update_ensure_connect_interval = [=] {
        auto interval_sec = ui->spinBox_ensure_connect_interval->value();
        ensure_connect_timer->setInterval(interval_sec * 1000);
    };

    auto sync_timer_running = [=] {
        const bool checked = ui->toolButton_ensure_connect->isChecked();
        if (checked && EnsureConnectPolicy::canRetry(ensure_connect_fail_count)) {
            if (!ensure_connect_timer->isActive()) {
                ensure_connect_timer->start();
            }
        } else {
            ensure_connect_timer->stop();
        }
    };

    auto restore_from_store = [=] {
        const int min_interval = ui->spinBox_ensure_connect_interval->minimum();
        const int max_interval = ui->spinBox_ensure_connect_interval->maximum();
        const int persisted_interval = std::clamp(NekoGui::dataStore->ensure_connect_interval_sec, min_interval, max_interval);
        ui->spinBox_ensure_connect_interval->setValue(persisted_interval);
        ensure_connect_fail_count = std::max(0, NekoGui::dataStore->ensure_connect_fail_count);
        ensure_connect_next_retry_at_ms = NekoGui::dataStore->ensure_connect_next_retry_at_ms;
        ensure_connect_last_failure_at_ms = NekoGui::dataStore->ensure_connect_last_failure_at_ms;
        ui->toolButton_ensure_connect->setChecked(NekoGui::dataStore->ensure_connect_enabled);
        update_ensure_connect_interval();
    };

    restore_from_store();

    connect(ui->spinBox_ensure_connect_interval,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [=](int) {
                update_ensure_connect_interval();
                persist_ensure_connect_state();
                sync_timer_running();
            });

    connect(ui->toolButton_ensure_connect, &QToolButton::toggled, this, [=](bool checked) {
        if (checked) {
            ensure_connect_fail_count = 0;
            ensure_connect_next_retry_at_ms = 0;
            ensure_connect_last_failure_at_ms = 0;
            ensure_connect_inflight = false;
            ensure_connect_target_id = -1;
            update_ensure_connect_interval();
        }
        persist_ensure_connect_state();
        sync_timer_running();
        if (!checked) ensure_connect_eval_timer->stop();
    });

    connect(ensure_connect_timer, &QTimer::timeout, this, [=] {
        if (ensure_connect_inflight) return;
        const auto now_ms = QDateTime::currentMSecsSinceEpoch();
        if (!EnsureConnectPolicy::shouldRunNow(ensure_connect_fail_count, ensure_connect_next_retry_at_ms, now_ms)) {
            sync_timer_running();
            return;
        }
        auto selected = get_now_selected_list();
        if (selected.isEmpty()) {
            return;
        }
        ensure_connect_target_id = selected.first()->id;
        ensure_connect_prev_latency = selected.first()->latency;
        ensure_connect_inflight = true;
        if (m_menuBuilder != nullptr && m_menuBuilder->menuServer() != nullptr) {
            auto prev = m_menuBuilder->menuServer()->property("selected_or_group");
            m_menuBuilder->menuServer()->setProperty("selected_or_group", 1);
            speedtest_current_group(1, false);
            m_menuBuilder->menuServer()->setProperty("selected_or_group", prev);
        } else {
            speedtest_current_group(1, false);
        }
        ensure_connect_eval_timer->start(12000);
    });

    connect(ensure_connect_eval_timer, &QTimer::timeout, this, [=] {
        ensure_connect_inflight = false;
        auto ent = NekoGui::profileManager->GetProfile(ensure_connect_target_id);
        if (ent == nullptr) {
            return;
        }
        if (ent->latency > 0) {
            EnsureConnectPolicy::markSuccess(ensure_connect_fail_count, ensure_connect_next_retry_at_ms);
            ensure_connect_last_failure_at_ms = 0;
            persist_ensure_connect_state();
            return;
        }
        ensure_connect_fail_count += 1;
        ensure_connect_last_failure_at_ms = QDateTime::currentMSecsSinceEpoch();
        if (EnsureConnectPolicy::canRetry(ensure_connect_fail_count)) {
            const int base_interval_sec = ui->spinBox_ensure_connect_interval->value();
            const int delay_sec = EnsureConnectPolicy::nextDelaySec(base_interval_sec, ensure_connect_fail_count);
            ensure_connect_next_retry_at_ms = ensure_connect_last_failure_at_ms + delay_sec * 1000LL;
            if (NekoGui::dataStore->started_id >= 0) {
                neko_start(NekoGui::dataStore->started_id);
            }
        } else {
            ensure_connect_next_retry_at_ms = 0;
            sync_timer_running();
        }
        persist_ensure_connect_state();
    });

    sync_timer_running();
}
