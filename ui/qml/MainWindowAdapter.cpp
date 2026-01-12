#include "MainWindowAdapter.hpp"
#include "main/NekoGui.hpp"
#include "main/NekoGui_Utils.hpp"
#include "ui/mainwindow_interface.h"
#include "db/Database.hpp"

// Adapter functions - these bridge to existing MainWindow functionality
// During migration, we keep MainWindow but hide it, using it only for core logic

void neko_start_impl(int id) {
    if (GetMainWindow()) {
        GetMainWindow()->neko_start(id);
    }
}

void neko_stop_impl(bool crash) {
    if (GetMainWindow()) {
        GetMainWindow()->neko_stop(crash, false);
    }
}

void refresh_proxy_list_impl(int id) {
    if (GetMainWindow()) {
        GetMainWindow()->refresh_proxy_list(id);
    }
}

void show_group_impl(int gid) {
    if (GetMainWindow()) {
        GetMainWindow()->show_group(gid);
    }
}

void refresh_groups_impl() {
    if (GetMainWindow()) {
        GetMainWindow()->refresh_groups();
    }
}

void refresh_status_impl(const QString &traffic_update) {
    if (GetMainWindow()) {
        GetMainWindow()->refresh_status(traffic_update);
    }
}

void neko_set_spmode_system_proxy_impl(bool enable, bool save) {
    if (GetMainWindow()) {
        GetMainWindow()->neko_set_spmode_system_proxy(enable, save);
    }
}

void neko_set_spmode_vpn_impl(bool enable, bool save) {
    if (GetMainWindow()) {
        GetMainWindow()->neko_set_spmode_vpn(enable, save);
    }
}

void show_log_impl(const QString &log) {
    if (MW_show_log) {
        MW_show_log(log);
    }
}

void refresh_connection_list_impl(const QJsonArray &arr) {
    if (GetMainWindow()) {
        GetMainWindow()->refresh_connection_list(arr);
    }
}

void speedtest_current_group_impl(int mode, bool test_group) {
    Q_UNUSED(mode)
    Q_UNUSED(test_group)
    // Speedtest UI is not wired for QML yet
}

void speedtest_current_impl() {
    // Speedtest UI is not wired for QML yet
}
