#define MW_INTERFACE

#include "mainwindow.h"
#include "main/NekoGui.hpp"
#include "main/NekoGui_Utils.hpp"
#include "db/Database.hpp"

#include <QJsonArray>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    mainwindow = this;
}

MainWindow::~MainWindow() = default;

void MainWindow::refresh_proxy_list(const int &id) {
    Q_UNUSED(id)
    emit profile_selected(id);
}

void MainWindow::show_group(int gid) {
    Q_UNUSED(gid)
}

void MainWindow::refresh_groups() {
}

void MainWindow::refresh_status(const QString &traffic_update) {
    Q_UNUSED(traffic_update)
}

void MainWindow::neko_start(int _id) {
    NekoGui::dataStore->started_id = _id;
}

void MainWindow::neko_stop(bool crash, bool sem) {
    Q_UNUSED(crash)
    Q_UNUSED(sem)
    NekoGui::dataStore->started_id = -1;
}

void MainWindow::neko_set_spmode_system_proxy(bool enable, bool save) {
    Q_UNUSED(enable)
    Q_UNUSED(save)
}

void MainWindow::neko_set_spmode_vpn(bool enable, bool save) {
    Q_UNUSED(enable)
    Q_UNUSED(save)
}

void MainWindow::show_log_impl(const QString &log) {
    if (MW_show_log) {
        MW_show_log(log);
    }
}

void MainWindow::start_select_mode(QObject *context, const std::function<void(int)> &callback) {
    Q_UNUSED(context)
    if (callback) {
        callback(-1);
    }
}

void MainWindow::refresh_connection_list(const QJsonArray &arr) {
    Q_UNUSED(arr)
}

void MainWindow::RegisterHotkey(bool unregister) {
    Q_UNUSED(unregister)
}

bool MainWindow::StopVPNProcess(bool unconditional) {
    Q_UNUSED(unconditional)
    return true;
}

void MainWindow::on_commitDataRequest() {
    NekoGui::dataStore->Save();
}

void MainWindow::on_menu_exit_triggered() {
    if (qApp) {
        qApp->quit();
    }
}

void UI_InitMainWindow() {
    if (!GetMainWindow()) {
        new MainWindow();
    }
}
