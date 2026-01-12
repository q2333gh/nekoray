#pragma once

#include <QString>
#include <QJsonArray>

// Adapter functions to bridge existing MainWindow code to QML
// These will be implemented by forwarding to existing MainWindow methods

void neko_start_impl(int id);
void neko_stop_impl(bool crash = false);
void refresh_proxy_list_impl(int id = -1);
void show_group_impl(int gid);
void refresh_groups_impl();
void refresh_status_impl(const QString &traffic_update = "");
void neko_set_spmode_system_proxy_impl(bool enable, bool save = true);
void neko_set_spmode_vpn_impl(bool enable, bool save = true);
void show_log_impl(const QString &log);
void refresh_connection_list_impl(const QJsonArray &arr);
void speedtest_current_group_impl(int mode, bool test_group);
void speedtest_current_impl();
