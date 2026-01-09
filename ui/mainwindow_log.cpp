#include "mainwindow.h"

#include <QTextCursor>
#include <QInputDialog>
#include <QMenu>

inline void FastAppendTextDocument(const QString &message, QTextDocument *doc) {
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertBlock();
    cursor.insertText(message);
    cursor.endEditBlock();
}

void MainWindow::show_log_impl(const QString &log) {
    auto lines = SplitLines(log.trimmed());
    if (lines.isEmpty()) return;

    QStringList newLines;
    auto log_ignore = NekoGui::dataStore->log_ignore;
    for (const auto &line: lines) {
        bool showThisLine = true;
        for (const auto &str: log_ignore) {
            if (line.contains(str)) {
                showThisLine = false;
                break;
            }
        }
        if (showThisLine) newLines << line;
    }
    if (newLines.isEmpty()) return;

    FastAppendTextDocument(newLines.join("\n"), qvLogDocument);
    auto block = qvLogDocument->begin();

    while (block.isValid()) {
        if (qvLogDocument->blockCount() > NekoGui::dataStore->max_log_line) {
            QTextCursor cursor(block);
            block = block.next();
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            continue;
        }
        break;
    }
}

#define ADD_TO_CURRENT_ROUTE(a, b)                                                                   \
    NekoGui::dataStore->routing->a = (SplitLines(NekoGui::dataStore->routing->a) << (b)).join("\n"); \
    NekoGui::dataStore->routing->Save();

void MainWindow::on_masterLogBrowser_customContextMenuRequested(const QPoint &pos) {
    QMenu *menu = ui->masterLogBrowser->createStandardContextMenu();

    auto sep = new QAction(this);
    sep->setSeparator(true);
    menu->addAction(sep);

    auto action_add_ignore = new QAction(this);
    action_add_ignore->setText(tr("Set ignore keyword"));
    connect(action_add_ignore, &QAction::triggered, this, [=] {
        auto list = NekoGui::dataStore->log_ignore;
        auto newStr = ui->masterLogBrowser->textCursor().selectedText().trimmed();
        if (!newStr.isEmpty()) list << newStr;
        bool ok;
        newStr = QInputDialog::getMultiLineText(GetMessageBoxParent(), tr("Set ignore keyword"), tr("Set the following keywords to ignore?\nSplit by line."), list.join("\n"), &ok);
        if (ok) {
            NekoGui::dataStore->log_ignore = SplitLines(newStr);
            NekoGui::dataStore->Save();
        }
    });
    menu->addAction(action_add_ignore);

    auto action_add_route = new QAction(this);
    action_add_route->setText(tr("Save as route"));
    connect(action_add_route, &QAction::triggered, this, [=] {
        auto newStr = ui->masterLogBrowser->textCursor().selectedText().trimmed();
        if (newStr.isEmpty()) return;
        //
        bool ok;
        newStr = QInputDialog::getText(GetMessageBoxParent(), tr("Save as route"), tr("Edit"), {}, newStr, &ok).trimmed();
        if (!ok) return;
        if (newStr.isEmpty()) return;
        //
        auto select = IsIpAddress(newStr) ? 0 : 3;
        QStringList items = {"proxyIP", "bypassIP", "blockIP", "proxyDomain", "bypassDomain", "blockDomain"};
        auto item = QInputDialog::getItem(GetMessageBoxParent(), tr("Save as route"),
                                          tr("Save \"%1\" as a routing rule?").arg(newStr),
                                          items, select, false, &ok);
        if (ok) {
            auto index = items.indexOf(item);
            switch (index) {
                case 0:
                    ADD_TO_CURRENT_ROUTE(proxy_ip, newStr);
                    break;
                case 1:
                    ADD_TO_CURRENT_ROUTE(direct_ip, newStr);
                    break;
                case 2:
                    ADD_TO_CURRENT_ROUTE(block_ip, newStr);
                    break;
                case 3:
                    ADD_TO_CURRENT_ROUTE(proxy_domain, newStr);
                    break;
                case 4:
                    ADD_TO_CURRENT_ROUTE(direct_domain, newStr);
                    break;
                case 5:
                    ADD_TO_CURRENT_ROUTE(block_domain, newStr);
                    break;
                default:
                    break;
            }
            MW_dialog_message("", "UpdateDataStore,RouteChanged");
        }
    });
    menu->addAction(action_add_route);

    auto action_clear = new QAction(this);
    action_clear->setText(tr("Clear"));
    connect(action_clear, &QAction::triggered, this, [=] {
        qvLogDocument->clear();
        ui->masterLogBrowser->clear();
    });
    menu->addAction(action_clear);

    menu->exec(ui->masterLogBrowser->viewport()->mapToGlobal(pos));
}
