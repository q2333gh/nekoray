#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include "mainwindow_menubuilder.h"

#include "db/Database.hpp"

#include <QSpinBox>
#include <QTimer>

void MainWindow::setup_ensure_connect_controls() {
    ensure_connect_timer = new QTimer(this);
    ensure_connect_timer->setSingleShot(false);
    ensure_connect_eval_timer = new QTimer(this);
    ensure_connect_eval_timer->setSingleShot(true);

    auto update_ensure_connect_interval = [=] {
        auto interval_sec = ui->spinBox_ensure_connect_interval->value();
        ensure_connect_timer->setInterval(interval_sec * 1000);
    };
    update_ensure_connect_interval();

    connect(ui->spinBox_ensure_connect_interval,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [=](int) {
                update_ensure_connect_interval();
                if (ui->toolButton_ensure_connect->isChecked() && !ensure_connect_timer->isActive()) {
                    ensure_connect_timer->start();
                }
            });

    connect(ui->toolButton_ensure_connect, &QToolButton::toggled, this, [=](bool checked) {
        ensure_connect_fail_count = 0;
        ensure_connect_inflight = false;
        ensure_connect_target_id = -1;
        if (checked) {
            update_ensure_connect_interval();
            ensure_connect_timer->start();
        } else {
            ensure_connect_timer->stop();
            ensure_connect_eval_timer->stop();
        }
    });

    connect(ensure_connect_timer, &QTimer::timeout, this, [=] {
        if (ensure_connect_inflight) return;
        auto selected = get_now_selected_list();
        if (selected.isEmpty()) {
            ensure_connect_fail_count = 0;
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
            ensure_connect_fail_count = 0;
            return;
        }
        if (ent->latency > 0) {
            ensure_connect_fail_count = 0;
            return;
        }
        ensure_connect_fail_count += 1;
        if (ensure_connect_fail_count == 1) {
            if (NekoGui::dataStore->started_id >= 0) {
                neko_start(NekoGui::dataStore->started_id);
            }
        } else {
            MW_dialog_message("", "RestartProgram");
            ensure_connect_fail_count = 0;
        }
    });
}
