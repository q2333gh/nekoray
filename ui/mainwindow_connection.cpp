#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include "ui/Icon.hpp"

#include <QLabel>
#include <QJsonArray>
#include <QTableWidgetItem>

inline QJsonArray last_arr; // format is nekoray_connections_json

static QJsonValue pick_first_key(const QJsonObject &obj, const QStringList &keys) {
    for (const auto &k: keys) {
        if (obj.contains(k)) return obj.value(k);
    }
    return {};
}

static void show_connection_hint_table(QTableWidget *table, const QString &outboundText, const QString &destText) {
    table->setRowCount(1);
    auto status = new QTableWidgetItem("-");
    status->setFlags(status->flags() & ~Qt::ItemIsEditable);
    table->setItem(0, 0, status);

    auto outbound = new QTableWidgetItem(outboundText);
    outbound->setFlags(outbound->flags() & ~Qt::ItemIsEditable);
    table->setItem(0, 1, outbound);

    auto dest = new QTableWidgetItem(destText);
    dest->setFlags(dest->flags() & ~Qt::ItemIsEditable);
    table->setItem(0, 2, dest);
}

void MainWindow::refresh_connection_list(const QJsonArray &arr) {
    if (!NekoGui::dataStore->connection_statistics) {
        last_arr = {};
        show_connection_hint_table(
            ui->tableWidget_conn,
            tr("Connection statistics is disabled"),
            tr("Enable it in Basic Settings -> Connection statistics"));
        return;
    }

    // Keep existing optimization for non-empty payloads.
    // For empty payload, still render hint to avoid an always-blank table.
    if (!arr.isEmpty() && last_arr == arr) {
        return;
    }
    last_arr = arr;

    if (NekoGui::dataStore->flag_debug) qDebug() << arr;

    ui->tableWidget_conn->setRowCount(0);
    if (arr.isEmpty()) {
        show_connection_hint_table(
            ui->tableWidget_conn,
            tr("No connection records"),
            tr("No active connections, or current core build does not support ListConnections"));
        return;
    }

    int row = -1;
    for (const auto &_item: arr) {
        auto item = _item.toObject();
        auto outboundTag = pick_first_key(item, {"Tag", "tag", "Outbound", "outbound"}).toString();
        if (NekoGui::dataStore->ignoreConnTag.contains(outboundTag)) continue;

        row++;
        ui->tableWidget_conn->insertRow(row);

        auto f0 = std::make_unique<QTableWidgetItem>();
        f0->setData(114514, pick_first_key(item, {"ID", "Id", "id"}).toInt());

        // C0: Status
        auto c0 = new QLabel;
        auto start_t = pick_first_key(item, {"Start", "start"}).toInt();
        auto end_t = pick_first_key(item, {"End", "end"}).toInt();
        // icon
        if (outboundTag == "block") {
            c0->setPixmap(Icon::GetMaterialIcon("cancel"));
        } else {
            if (end_t > 0) {
                c0->setPixmap(Icon::GetMaterialIcon("history"));
            } else {
                c0->setPixmap(Icon::GetMaterialIcon("swap-vertical"));
            }
        }
        c0->setAlignment(Qt::AlignCenter);
        c0->setToolTip(tr("Start: %1\nEnd: %2").arg(DisplayTime(start_t), end_t > 0 ? DisplayTime(end_t) : ""));
        ui->tableWidget_conn->setCellWidget(row, 0, c0);

        // C1: Outbound
        auto f = f0->clone();
        f->setToolTip("");
        f->setText(outboundTag);
        ui->tableWidget_conn->setItem(row, 1, f);

        // C2: Destination
        f = f0->clone();
        QString target1 = pick_first_key(item, {"Dest", "dest", "Destination", "destination"}).toString();
        QString target2 = pick_first_key(item, {"RDest", "rDest", "rdest", "ResolvedDest", "resolvedDest"}).toString();
        if (target2.isEmpty() || target1 == target2) {
            target2 = "";
        }
        f->setText("[" + target1 + "] " + target2);
        ui->tableWidget_conn->setItem(row, 2, f);
    }

    if (row < 0) {
        show_connection_hint_table(
            ui->tableWidget_conn,
            tr("No visible connection records"),
            tr("All records may be filtered by ignoreConnTag"));
    }
}
