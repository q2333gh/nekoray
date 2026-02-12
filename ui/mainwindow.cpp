#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include "mainwindow_menubuilder.h"

#include "fmt/Preset.hpp"
#include "db/ProfileFilter.hpp"
#include "db/ConfigBuilder.hpp"
#include "sub/GroupUpdater.hpp"
#include "sys/ExternalProcess.hpp"
#include "sys/AutoRun.hpp"

#include "ui/ThemeManager.hpp"
#include "ui/Icon.hpp"
#include "ui/edit/dialog_edit_profile.h"
#include "ui/dialog_basic_settings.h"
#include "ui/dialog_manage_groups.h"
#include "ui/dialog_manage_routes.h"
#include "ui/dialog_vpn_settings.h"
#include "ui/dialog_hotkey.h"

#include "3rdparty/fix_old_qt.h"
#include "3rdparty/qrcodegen.hpp"
#include "3rdparty/VT100Parser.hpp"
#include "3rdparty/qv2ray/v2/components/proxy/QvProxyConfigurator.hpp"

#ifndef NKR_NO_ZXING
#include "3rdparty/ZxingQtReader.hpp"
#endif

#ifdef Q_OS_WIN
#include "3rdparty/WinCommander.hpp"
#else
#ifdef Q_OS_LINUX
#include "sys/linux/LinuxCap.h"
#endif
#include <unistd.h>
#endif

#include <QClipboard>
#include <QLabel>
#include <QTextBlock>
#include <QScrollBar>
#include <QScreen>
#include <QDesktopServices>
#include <QInputDialog>
#include <QThread>
#include <QTimer>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QIODevice>

void UI_InitMainWindow() {
    mainwindow = new MainWindow;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    mainwindow = this;
    MW_dialog_message = [=](const QString &a, const QString &b) {
        runOnUiThread([=] { dialog_message_impl(a, b); });
    };

    // Load Manager
    NekoGui::profileManager->LoadManager();

    // Setup misc UI
    themeManager->ApplyTheme(NekoGui::dataStore->theme);
    ui->setupUi(this);
    
    // Build menus dynamically
    m_menuBuilder = new MenuBuilder(this);
    m_menuBuilder->buildMenus(ui->menubar);
    addActions(m_menuBuilder->getAllActions());
    
    //
    connect(m_menuBuilder->actionStart(), &QAction::triggered, this, [=]() {
        // Log before starting
        QString logPath = QDir::currentPath() + "/crash_log.txt";
        QFile logFile(logPath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << " - menu_start triggered, calling neko_start()\n";
            logFile.close();
        }
        neko_start();
    });
    connect(m_menuBuilder->actionStop(), &QAction::triggered, this, [=]() { neko_stop(); });
    connect(ui->tabWidget->tabBar(), &QTabBar::tabMoved, this, [=](int from, int to) {
        // use tabData to track tab & gid
        NekoGui::profileManager->groupsTabOrder.clear();
        for (int i = 0; i < ui->tabWidget->tabBar()->count(); i++) {
            NekoGui::profileManager->groupsTabOrder += ui->tabWidget->tabBar()->tabData(i).toInt();
        }
        NekoGui::profileManager->SaveManager();
    });
    ui->label_running->installEventFilter(this);
    ui->label_inbound->installEventFilter(this);
    ui->splitter->installEventFilter(this);
    //
    RegisterHotkey(false);
    //
    auto last_size = NekoGui::dataStore->mw_size.split("x");
    if (last_size.length() == 2) {
        auto w = last_size[0].toInt();
        auto h = last_size[1].toInt();
        if (w > 0 && h > 0) {
            resize(w, h);
        }
    }

    if (QDir("dashboard").count() == 0) {
        QDir().mkdir("dashboard");
        QFile::copy(":/neko/dashboard-notice.html", "dashboard/index.html");
    }

    // top bar
    ui->toolButton_program->setMenu(m_menuBuilder->menuProgram());
    ui->toolButton_preferences->setMenu(m_menuBuilder->menuPreferences());
    ui->toolButton_server->setMenu(m_menuBuilder->menuServer());
    ui->menubar->setVisible(false);
    connect(ui->toolButton_document, &QToolButton::clicked, this, [=] { QDesktopServices::openUrl(QUrl("https://matsuridayo.github.io/")); });

    connect(ui->toolButton_update, &QToolButton::clicked, this, [=] { runOnNewThread([=] { CheckUpdate(); }); });
    connect(ui->toolButton_url_test, &QToolButton::clicked, this, [=] { speedtest_current_group(1, true); });
    connect(ui->toolButton_restart_server, &QToolButton::clicked, this, [=] {
        if (NekoGui::dataStore->started_id >= 0) neko_start(NekoGui::dataStore->started_id);
    });
    connect(ui->toolButton_restart_program, &QToolButton::clicked, this, [=] { MW_dialog_message("", "RestartProgram"); });
    setup_ensure_connect_controls();
    
    // Set AccessibleName for WinAppDriver automation (only for QWidget, not QAction)
    ui->toolButton_url_test->setAccessibleName("URLTestButton");
    ui->proxyListTable->setAccessibleName("ProxyListTable");
    this->setAccessibleName("MainWindow");

    // Setup log UI
    ui->splitter->restoreState(DecodeB64IfValid(NekoGui::dataStore->splitter_state));
    qvLogDocument->setUndoRedoEnabled(false);
    ui->masterLogBrowser->setUndoRedoEnabled(false);
    ui->masterLogBrowser->setDocument(qvLogDocument);
    ui->masterLogBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    {
        auto font = ui->masterLogBrowser->font();
        font.setPointSize(9);
        ui->masterLogBrowser->setFont(font);
        qvLogDocument->setDefaultFont(font);
    }
    connect(ui->masterLogBrowser->verticalScrollBar(), &QSlider::valueChanged, this, [=](int value) {
        if (ui->masterLogBrowser->verticalScrollBar()->maximum() == value)
            qvLogAutoScoll = true;
        else
            qvLogAutoScoll = false;
    });
    connect(ui->masterLogBrowser, &QTextBrowser::textChanged, this, [=]() {
        if (!qvLogAutoScoll)
            return;
        auto bar = ui->masterLogBrowser->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
    MW_show_log = [=](const QString &log) {
        runOnUiThread([=] { show_log_impl(log); });
    };
    MW_show_log_ext = [=](const QString &tag, const QString &log) {
        runOnUiThread([=] { show_log_impl("[" + tag + "] " + log); });
    };
    MW_show_log_ext_vt100 = [=](const QString &log) {
        runOnUiThread([=] { show_log_impl(cleanVT100String(log)); });
    };

    // table UI
    ui->proxyListTable->callback_save_order = [=] {
        auto group = NekoGui::profileManager->CurrentGroup();
        group->order = ui->proxyListTable->order;
        group->Save();
    };
    ui->proxyListTable->refresh_data = [=](int id) { refresh_proxy_list_impl_refresh_data(id); };
    connect(ui->proxyListTable, &QTableWidget::itemChanged, this, &MainWindow::on_proxyListTable_itemChanged);
    if (auto button = ui->proxyListTable->findChild<QAbstractButton *>(QString(), Qt::FindDirectChildrenOnly)) {
        // Corner Button
        connect(button, &QAbstractButton::clicked, this, [=] { refresh_proxy_list_impl(-1, {GroupSortMethod::ById}); });
    }
    connect(ui->proxyListTable->horizontalHeader(), &QHeaderView::sectionClicked, this, [=](int logicalIndex) {
        if (logicalIndex < 1 || logicalIndex > 4) return;
        GroupSortAction action;
        // ????descending??
        if (proxy_last_order == logicalIndex) {
            action.descending = true;
            proxy_last_order = -1;
        } else {
            proxy_last_order = logicalIndex;
        }
        action.save_sort = true;
        // ??
        if (logicalIndex == 1) {
            action.method = GroupSortMethod::ByType;
        } else if (logicalIndex == 2) {
            action.method = GroupSortMethod::ByAddress;
        } else if (logicalIndex == 3) {
            action.method = GroupSortMethod::ByName;
        } else if (logicalIndex == 4) {
            action.method = GroupSortMethod::ByLatency;
        }
        refresh_proxy_list_impl(-1, action);
    });
    connect(ui->proxyListTable->horizontalHeader(), &QHeaderView::sectionResized, this, [=](int logicalIndex, int oldSize, int newSize) {
        auto group = NekoGui::profileManager->CurrentGroup();
        if (NekoGui::dataStore->refreshing_group || group == nullptr || !group->manually_column_width) return;
        // save manually column width
        group->column_width.clear();
        for (int i = 0; i < ui->proxyListTable->horizontalHeader()->count(); i++) {
            group->column_width.push_back(ui->proxyListTable->horizontalHeader()->sectionSize(i));
        }
        group->column_width[logicalIndex] = newSize;
        group->Save();
    });
    ui->tableWidget_conn->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget_conn->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget_conn->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    connect(ui->down_tab, &QTabWidget::currentChanged, this, [=](int index) {
        // Connection tab: render either data rows or informative placeholder.
        if (index == 1) {
            refresh_connection_list({});
        }
    });
    ui->proxyListTable->verticalHeader()->setDefaultSectionSize(24);

    // search box
    ui->search->setVisible(false);
    connect(shortcut_ctrl_f, &QShortcut::activated, this, [=] {
        ui->search->setVisible(true);
        ui->search->setFocus();
    });
    connect(shortcut_esc, &QShortcut::activated, this, [=] {
        if (ui->search->isVisible()) {
            ui->search->setText("");
            ui->search->textChanged("");
            ui->search->setVisible(false);
        }
        if (select_mode) {
            emit profile_selected(-1);
            select_mode = false;
            refresh_status();
        }
    });
    connect(ui->search, &QLineEdit::textChanged, this, [=](const QString &text) {
        if (text.isEmpty()) {
            for (int i = 0; i < ui->proxyListTable->rowCount(); i++) {
                ui->proxyListTable->setRowHidden(i, false);
            }
        } else {
            QList<QTableWidgetItem *> findItem = ui->proxyListTable->findItems(text, Qt::MatchContains);
            for (int i = 0; i < ui->proxyListTable->rowCount(); i++) {
                ui->proxyListTable->setRowHidden(i, true);
            }
            for (auto item: findItem) {
                if (item != nullptr) ui->proxyListTable->setRowHidden(item->row(), false);
            }
        }
    });

    // refresh
    this->refresh_groups();

    // Setup Tray
    tray = new QSystemTrayIcon(this); // 鍒濆鍖栨墭鐩樺璞ray
    tray->setIcon(Icon::GetTrayIcon(Icon::NONE));
    tray->setContextMenu(m_menuBuilder->menuProgram()); // 鍒涘缓鎵樼洏鑿滃崟
    tray->show();                           // 璁╂墭鐩樺浘鏍囨樉绀哄湪绯荤粺鎵樼洏涓?
    connect(tray, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (this->isVisible()) {
                hide();
            } else {
                ActivateWindow(this);
            }
        }
    });

    // Misc menu
    connect(m_menuBuilder->actionOpenConfigFolder(), &QAction::triggered, this, [=] { QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::currentPath())); });
    connect(m_menuBuilder->actionAddFromClipboard2(), &QAction::triggered, m_menuBuilder->actionAddFromClipboard(), &QAction::trigger);
    connect(m_menuBuilder->actionRestartProxy(), &QAction::triggered, this, [=] { if (NekoGui::dataStore->started_id>=0) neko_start(NekoGui::dataStore->started_id); });
    connect(m_menuBuilder->actionRestartProgram(), &QAction::triggered, this, [=] { MW_dialog_message("", "RestartProgram"); });
    connect(m_menuBuilder->actionShowWindow(), &QAction::triggered, this, [=] { tray->activated(QSystemTrayIcon::ActivationReason::Trigger); });
    connect(m_menuBuilder->actionExit(), &QAction::triggered, this, &MainWindow::on_menu_exit_triggered);
    connect(m_menuBuilder->actionManageGroups(), &QAction::triggered, this, &MainWindow::on_menu_manage_groups_triggered);
    connect(m_menuBuilder->actionBasicSettings(), &QAction::triggered, this, &MainWindow::on_menu_basic_settings_triggered);
    connect(m_menuBuilder->actionRoutingSettings(), &QAction::triggered, this, &MainWindow::on_menu_routing_settings_triggered);
    connect(m_menuBuilder->actionVPNSettings(), &QAction::triggered, this, &MainWindow::on_menu_vpn_settings_triggered);
    connect(m_menuBuilder->actionHotkeySettings(), &QAction::triggered, this, &MainWindow::on_menu_hotkey_settings_triggered);
    connect(m_menuBuilder->actionAddFromInput(), &QAction::triggered, this, &MainWindow::on_menu_add_from_input_triggered);
    connect(m_menuBuilder->actionAddFromClipboard(), &QAction::triggered, this, &MainWindow::on_menu_add_from_clipboard_triggered);
    connect(m_menuBuilder->actionScanQR(), &QAction::triggered, this, &MainWindow::on_menu_scan_qr_triggered);
    connect(m_menuBuilder->actionSelectAll(), &QAction::triggered, this, &MainWindow::on_menu_select_all_triggered);
    connect(m_menuBuilder->actionMove(), &QAction::triggered, this, &MainWindow::on_menu_move_triggered);
    connect(m_menuBuilder->actionClone(), &QAction::triggered, this, &MainWindow::on_menu_clone_triggered);
    connect(m_menuBuilder->actionResetTraffic(), &QAction::triggered, this, &MainWindow::on_menu_reset_traffic_triggered);
    connect(m_menuBuilder->actionDelete(), &QAction::triggered, this, &MainWindow::on_menu_delete_triggered);
    connect(m_menuBuilder->actionCopyLinks(), &QAction::triggered, this, &MainWindow::on_menu_copy_links_triggered);
    connect(m_menuBuilder->actionCopyLinksNKR(), &QAction::triggered, this, &MainWindow::on_menu_copy_links_nkr_triggered);
    connect(m_menuBuilder->actionExportConfig(), &QAction::triggered, this, &MainWindow::on_menu_export_config_triggered);
    connect(m_menuBuilder->actionProfileDebugInfo(), &QAction::triggered, this, &MainWindow::on_menu_profile_debug_info_triggered);
    connect(m_menuBuilder->actionClearTestResult(), &QAction::triggered, this, &MainWindow::on_menu_clear_test_result_triggered);
    connect(m_menuBuilder->actionResolveDomain(), &QAction::triggered, this, &MainWindow::on_menu_resolve_domain_triggered);
    connect(m_menuBuilder->actionRemoveUnavailable(), &QAction::triggered, this, &MainWindow::on_menu_remove_unavailable_triggered);
    connect(m_menuBuilder->actionDeleteRepeat(), &QAction::triggered, this, &MainWindow::on_menu_delete_repeat_triggered);
    connect(m_menuBuilder->actionUpdateSubscription(), &QAction::triggered, this, &MainWindow::on_menu_update_subscription_triggered);
    //
    connect(m_menuBuilder->menuProgram(), &QMenu::aboutToShow, this, [=]() {
        m_menuBuilder->actionRememberLastProxy()->setChecked(NekoGui::dataStore->remember_enable);
        m_menuBuilder->actionStartWithSystem()->setChecked(AutoRun_IsEnabled());
        m_menuBuilder->actionAllowLAN()->setChecked(QStringList{"::", "0.0.0.0"}.contains(NekoGui::dataStore->inbound_address));
        // active server
        for (const auto &old: m_menuBuilder->menuActiveServer()->actions()) {
            m_menuBuilder->menuActiveServer()->removeAction(old);
            old->deleteLater();
        }
        int active_server_item_count = 0;
        for (const auto &pf: NekoGui::profileManager->CurrentGroup()->ProfilesWithOrder()) {
            auto a = new QAction(pf->bean->DisplayTypeAndName(), this);
            a->setProperty("id", pf->id);
            a->setCheckable(true);
            if (NekoGui::dataStore->started_id == pf->id) a->setChecked(true);
            m_menuBuilder->menuActiveServer()->addAction(a);
            if (++active_server_item_count == 100) break;
        }
        // active routing
        for (const auto &old: m_menuBuilder->menuActiveRouting()->actions()) {
            m_menuBuilder->menuActiveRouting()->removeAction(old);
            old->deleteLater();
        }
        for (const auto &name: NekoGui::Routing::List()) {
            auto a = new QAction(name, this);
            a->setCheckable(true);
            a->setChecked(name == NekoGui::dataStore->active_routing);
            m_menuBuilder->menuActiveRouting()->addAction(a);
        }
    });
    connect(m_menuBuilder->menuActiveServer(), &QMenu::triggered, this, [=](QAction *a) {
        bool ok;
        auto id = a->property("id").toInt(&ok);
        if (!ok) return;
        if (NekoGui::dataStore->started_id == id) {
            neko_stop();
        } else {
            neko_start(id);
        }
    });
    connect(m_menuBuilder->menuActiveRouting(), &QMenu::triggered, this, [=](QAction *a) {
        auto fn = a->text();
        if (!fn.isEmpty()) {
            NekoGui::Routing r;
            r.load_control_must = true;
            r.fn = ROUTES_PREFIX + fn;
            if (r.Load()) {
                if (QMessageBox::question(GetMessageBoxParent(), software_name, tr("Load routing and apply: %1").arg(fn) + "\n" + r.DisplayRouting()) == QMessageBox::Yes) {
                    NekoGui::Routing::SetToActive(fn);
                    if (NekoGui::dataStore->started_id >= 0) {
                        neko_start(NekoGui::dataStore->started_id);
                    } else {
                        refresh_status();
                    }
                }
            }
        }
    });
    connect(m_menuBuilder->actionRememberLastProxy(), &QAction::triggered, this, [=](bool checked) {
        NekoGui::dataStore->remember_enable = checked;
        NekoGui::dataStore->Save();
    });
    connect(m_menuBuilder->actionStartWithSystem(), &QAction::triggered, this, [=](bool checked) {
        AutoRun_SetEnabled(checked);
    });
    connect(m_menuBuilder->actionAllowLAN(), &QAction::triggered, this, [=](bool checked) {
        NekoGui::dataStore->inbound_address = checked ? "::" : "127.0.0.1";
        MW_dialog_message("", "UpdateDataStore");
    });
    //
    connect(ui->checkBox_VPN, &QCheckBox::clicked, this, [=](bool checked) { neko_set_spmode_vpn(checked); });
    connect(ui->checkBox_SystemProxy, &QCheckBox::clicked, this, [=](bool checked) { neko_set_spmode_system_proxy(checked); });
    connect(m_menuBuilder->menuSpmode(), &QMenu::aboutToShow, this, [=]() {
        m_menuBuilder->actionSpmodeDisabled()->setChecked(!(NekoGui::dataStore->spmode_system_proxy || NekoGui::dataStore->spmode_vpn));
        m_menuBuilder->actionSpmodeSystemProxy()->setChecked(NekoGui::dataStore->spmode_system_proxy);
        m_menuBuilder->actionSpmodeVPN()->setChecked(NekoGui::dataStore->spmode_vpn);
    });
    connect(m_menuBuilder->actionSpmodeSystemProxy(), &QAction::triggered, this, [=](bool checked) { neko_set_spmode_system_proxy(checked); });
    connect(m_menuBuilder->actionSpmodeVPN(), &QAction::triggered, this, [=](bool checked) { neko_set_spmode_vpn(checked); });
    connect(m_menuBuilder->actionSpmodeDisabled(), &QAction::triggered, this, [=]() {
        neko_set_spmode_system_proxy(false);
        neko_set_spmode_vpn(false);
    });
    connect(m_menuBuilder->actionQR(), &QAction::triggered, this, [=]() { display_qr_link(false); });
    connect(m_menuBuilder->actionTCPPing(), &QAction::triggered, this, [=]() { speedtest_current_group(0, false); });
    connect(m_menuBuilder->actionURLTest(), &QAction::triggered, this, [=]() { speedtest_current_group(1, false); });
    connect(m_menuBuilder->actionFullTest(), &QAction::triggered, this, [=]() { speedtest_current_group(2, false); });
    connect(m_menuBuilder->actionStopTesting(), &QAction::triggered, this, [=]() { speedtest_current_group(114514, false); });
    //
    auto set_selected_or_group = [=](int mode) {
        // 0=group 1=select 2=unknown(menu is hide)
        m_menuBuilder->menuServer()->setProperty("selected_or_group", mode);
    };
    // Create fake actions for dynamic menu insertion
    auto fakeAction4 = new QAction("fake", this);
    fakeAction4->setVisible(false);
    auto fakeAction5 = new QAction("fake", this);
    fakeAction5->setVisible(false);
    m_menuBuilder->menuCurrentSelect()->addAction(fakeAction4);
    m_menuBuilder->menuCurrentGroup()->addAction(fakeAction5);
    
    auto move_tests_to_menu = [=](bool menuCurrent_Select) {
        return [=] {
            if (menuCurrent_Select) {
                m_menuBuilder->menuCurrentSelect()->insertAction(fakeAction4, m_menuBuilder->actionTCPPing());
                m_menuBuilder->menuCurrentSelect()->insertAction(fakeAction4, m_menuBuilder->actionURLTest());
                m_menuBuilder->menuCurrentSelect()->insertAction(fakeAction4, m_menuBuilder->actionFullTest());
                m_menuBuilder->menuCurrentSelect()->insertAction(fakeAction4, m_menuBuilder->actionStopTesting());
                m_menuBuilder->menuCurrentSelect()->insertAction(fakeAction4, m_menuBuilder->actionClearTestResult());
                m_menuBuilder->menuCurrentSelect()->insertAction(fakeAction4, m_menuBuilder->actionResolveDomain());
            } else {
                m_menuBuilder->menuCurrentGroup()->insertAction(fakeAction5, m_menuBuilder->actionTCPPing());
                m_menuBuilder->menuCurrentGroup()->insertAction(fakeAction5, m_menuBuilder->actionURLTest());
                m_menuBuilder->menuCurrentGroup()->insertAction(fakeAction5, m_menuBuilder->actionFullTest());
                m_menuBuilder->menuCurrentGroup()->insertAction(fakeAction5, m_menuBuilder->actionStopTesting());
                m_menuBuilder->menuCurrentGroup()->insertAction(fakeAction5, m_menuBuilder->actionClearTestResult());
                m_menuBuilder->menuCurrentGroup()->insertAction(fakeAction5, m_menuBuilder->actionResolveDomain());
            }
            set_selected_or_group(menuCurrent_Select ? 1 : 0);
        };
    };
    connect(m_menuBuilder->menuCurrentSelect(), &QMenu::aboutToShow, this, move_tests_to_menu(true));
    connect(m_menuBuilder->menuCurrentGroup(), &QMenu::aboutToShow, this, move_tests_to_menu(false));
    connect(m_menuBuilder->menuServer(), &QMenu::aboutToHide, this, [=] {
        setTimeout([=] { set_selected_or_group(2); }, this, 200);
    });
    set_selected_or_group(2);
    //
    connect(m_menuBuilder->menuShareItem(), &QMenu::aboutToShow, this, [=] {
        QString name;
        auto selected = get_now_selected_list();
        if (!selected.isEmpty()) {
            auto ent = selected.first();
            name = ent->bean->DisplayCoreType();
        }
        m_menuBuilder->actionExportConfig()->setVisible(name == software_core_name);
        m_menuBuilder->actionExportConfig()->setText(tr("Export %1 config").arg(name));
    });
    refresh_status();

    // Prepare core
    NekoGui::dataStore->core_token = GetRandomString(32);
    NekoGui::dataStore->core_port = MkPort();
    if (NekoGui::dataStore->core_port <= 0) NekoGui::dataStore->core_port = 19810;

    auto core_path = QApplication::applicationDirPath() + "/";
#ifdef Q_OS_WIN
    core_path += "nekobox_core.exe";
#else
    core_path += "nekobox_core";
#endif

    // Check if core executable exists
    QFileInfo coreFileInfo(core_path);
    if (!coreFileInfo.exists() || !coreFileInfo.isFile()) {
        // Write error log to file
        QString logDir = QDir::currentPath();
        QString logPath = logDir + "/core_not_found.log";
        QFile logFile(logPath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
            out << "ERROR: Core executable not found!\n";
            out << "Expected path: " << core_path << "\n";
            out << "Application directory: " << QApplication::applicationDirPath() << "\n";
            out << "Current working directory: " << QDir::currentPath() << "\n";
            out << "\n";
            out << "Attempting automatic download...\n";
            logFile.close();
        }
        
        // Try to download core automatically
        qCritical() << "Core executable not found:" << core_path;
        qCritical() << "Attempting automatic download...";
        
        // Show message to user
        QMessageBox::information(this, software_name, 
            QString("Core executable not found.\n\nAttempting to download automatically...\n\nThis may take a few minutes."));
        
        // Download core (this function needs to be accessible here)
        // For now, we'll use a simple approach: call the download script
#ifdef Q_OS_WIN
        QString scriptPath = QDir(QApplication::applicationDirPath()).absolutePath() + "/libs/download_core.py";
        QFileInfo scriptInfo(scriptPath);
        
        // Try to find script in repo
        if (!scriptInfo.exists()) {
            QDir dir(QApplication::applicationDirPath());
            for (int i = 0; i < 5 && !scriptInfo.exists(); i++) {
                dir.cdUp();
                scriptPath = dir.absolutePath() + "/libs/download_core.py";
                scriptInfo.setFile(scriptPath);
            }
        }
        
        if (scriptInfo.exists()) {
            QProcess downloadProcess;
            // Try to use python from PATH, or common locations
            QString pythonExe = "python";
            QStringList pythonPaths = {
                "python",
                "python3",
                "py",
                QApplication::applicationDirPath() + "/python.exe",
                "C:/Python312/python.exe",
                "C:/Python311/python.exe",
                "C:/Python310/python.exe"
            };
            
            // Find available Python
            for (const QString &pyPath : pythonPaths) {
                QProcess testProcess;
                testProcess.start(pyPath, QStringList() << "--version");
                if (testProcess.waitForFinished(1000) && testProcess.exitCode() == 0) {
                    pythonExe = pyPath;
                    break;
                }
            }
            
            downloadProcess.setProgram(pythonExe);
            QStringList args;
            args << scriptPath;
            args << "--dest-dir" << QApplication::applicationDirPath();
            args << "--version" << "latest";
            
            downloadProcess.setArguments(args);
            downloadProcess.setProcessChannelMode(QProcess::MergedChannels);
            
            qCritical() << "Starting download process...";
            downloadProcess.start();
            
            if (downloadProcess.waitForFinished(300000)) { // 5 minutes timeout
                int exitCode = downloadProcess.exitCode();
                QString output = QString::fromUtf8(downloadProcess.readAllStandardOutput());
                qCritical() << "Download finished with exit code:" << exitCode;
                qCritical() << "Output:" << output;
                
                if (exitCode == 0) {
                    // Verify downloaded file
                    QFileInfo downloadedInfo(core_path);
                    if (downloadedInfo.exists() && downloadedInfo.isFile()) {
                        QMessageBox::information(this, software_name, 
                            QString("Core downloaded successfully!\n\nPath: %1").arg(core_path));
                        // Continue with initialization
                    } else {
                        QMessageBox::warning(this, software_name, 
                            QString("Download completed but file not found.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases"));
                        QApplication::exit(1);
                        return;
                    }
                } else {
                    QMessageBox::warning(this, software_name, 
                        QString("Failed to download core executable.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases\n\nAnd place it in: %1").arg(QApplication::applicationDirPath()));
                    QApplication::exit(1);
                    return;
                }
            } else {
                QMessageBox::warning(this, software_name, 
                    QString("Download timed out.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases"));
                QApplication::exit(1);
                return;
            }
        } else {
            QMessageBox::warning(this, software_name, 
                QString("Download script not found.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases\n\nAnd place it in: %1").arg(QApplication::applicationDirPath()));
            QApplication::exit(1);
            return;
        }
#else
        QMessageBox::warning(this, software_name, 
            QString("Please ensure nekobox_core is present in the application directory."));
        QApplication::exit(1);
        return;
#endif
    }

    QStringList args;
    args.push_back("nekobox");
    args.push_back("-port");
    args.push_back(Int2String(NekoGui::dataStore->core_port));
    if (NekoGui::dataStore->flag_debug) args.push_back("-debug");

    // Start core
    runOnUiThread(
        [=] {
            core_process = new NekoGui_sys::CoreProcess(core_path, args);
            // Remember last started
            if (NekoGui::dataStore->remember_enable && NekoGui::dataStore->remember_id >= 0) {
                core_process->start_profile_when_core_is_up = NekoGui::dataStore->remember_id;
            }
            // Setup
            core_process->Start();
            setup_grpc();
        },
        DS_cores);

    // Remember system proxy
    if (NekoGui::dataStore->remember_enable || NekoGui::dataStore->flag_restart_tun_on) {
        if (NekoGui::dataStore->remember_spmode.contains("system_proxy")) {
            neko_set_spmode_system_proxy(true, false);
        }
        if (NekoGui::dataStore->remember_spmode.contains("vpn") || NekoGui::dataStore->flag_restart_tun_on) {
            neko_set_spmode_vpn(true, false);
        }
    }

    connect(qApp, &QGuiApplication::commitDataRequest, this, &MainWindow::on_commitDataRequest);

    auto t = new QTimer;
    connect(t, &QTimer::timeout, this, [=]() { refresh_status(); });
    t->start(2000);

    t = new QTimer;
    connect(t, &QTimer::timeout, this, [&] { NekoGui_sys::logCounter.fetchAndStoreRelaxed(0); });
    t->start(1000);

    TM_auto_update_subsctiption = new QTimer;
    TM_auto_update_subsctiption_Reset_Minute = [&](int m) {
        TM_auto_update_subsctiption->stop();
        if (m >= 30) TM_auto_update_subsctiption->start(m * 60 * 1000);
    };
    connect(TM_auto_update_subsctiption, &QTimer::timeout, this, [&] { UI_update_all_groups(true); });
    TM_auto_update_subsctiption_Reset_Minute(NekoGui::dataStore->sub_auto_update);

    if (!NekoGui::dataStore->flag_tray) show();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (tray->isVisible()) {
        hide();          // 闅愯棌绐楀彛
        event->ignore(); // 蹇界暐浜嬩欢
    }
}

MainWindow::~MainWindow() {
    delete m_menuBuilder;
    delete ui;
}

// Group tab manage - moved to mainwindow_proxy_list.cpp

// callback - moved to mainwindow_dialog.cpp

void MainWindow::on_commitDataRequest() {
    qDebug() << "Start of data save";
    // OS shutdown/restart may bypass on_menu_exit_triggered().
    // Best-effort clear system proxy to avoid stale proxy settings after reboot.
    if (NekoGui::dataStore->spmode_system_proxy) {
        neko_set_spmode_system_proxy(false, false);
    }
    //
    if (!isMaximized()) {
        auto olds = NekoGui::dataStore->mw_size;
        auto news = QStringLiteral("%1x%2").arg(size().width()).arg(size().height());
        if (olds != news) {
            NekoGui::dataStore->mw_size = news;
        }
    }
    //
    NekoGui::dataStore->splitter_state = ui->splitter->saveState().toBase64();
    //
    auto last_id = NekoGui::dataStore->started_id;
    if (NekoGui::dataStore->remember_enable && last_id >= 0) {
        NekoGui::dataStore->remember_id = last_id;
    }
    //
    NekoGui::dataStore->Save();
    NekoGui::profileManager->SaveManager();
    qDebug() << "End of data save";
}

void MainWindow::on_menu_exit_triggered() {
    if (mu_exit.tryLock()) {
        NekoGui::dataStore->prepare_exit = true;
        //
        neko_set_spmode_system_proxy(false, false);
        neko_set_spmode_vpn(false, false);
        if (NekoGui::dataStore->spmode_vpn) {
            mu_exit.unlock(); // retry
            return;
        }
        RegisterHotkey(true);
        //
        on_commitDataRequest();
        //
        NekoGui::dataStore->save_control_no_save = true; // don't change datastore after this line
        neko_stop(false, true);
        //
        hide();
        runOnNewThread([=] {
            sem_stopped.acquire();
            stop_core_daemon();
            runOnUiThread([=] {
                on_menu_exit_triggered(); // continue exit progress
            });
        });
        return;
    }
    //
    MF_release_runguard();
    if (exit_reason == 1) {
        QDir::setCurrent(QApplication::applicationDirPath());
        QProcess::startDetached("./updater", QStringList{});
    } else if (exit_reason == 2 || exit_reason == 3) {
        QDir::setCurrent(QApplication::applicationDirPath());

        auto arguments = NekoGui::dataStore->argv;
        if (arguments.length() > 0) {
            arguments.removeFirst();
            arguments.removeAll("-tray");
            arguments.removeAll("-flag_restart_tun_on");
            arguments.removeAll("-flag_reorder");
        }
        auto isLauncher = qEnvironmentVariable("NKR_FROM_LAUNCHER") == "1";
        if (isLauncher) arguments.prepend("--");
        auto program = isLauncher ? "./launcher" : QApplication::applicationFilePath();

        if (exit_reason == 3) {
            // Tun restart as admin
            arguments << "-flag_restart_tun_on";
#ifdef Q_OS_WIN
            WinCommander::runProcessElevated(program, arguments, "", WinCommander::SW_NORMAL, false);
#else
            QProcess::startDetached(program, arguments);
#endif
        } else {
            QProcess::startDetached(program, arguments);
        }
    }
    tray->hide();
    QCoreApplication::quit();
}

// VPN and system proxy - moved to mainwindow_vpn.cpp
// refresh_status - moved to mainwindow_status.cpp
// refresh_groups, refresh_proxy_list, on_proxyListTable_* - moved to mainwindow_proxy_list.cpp
// on_menu_* functions - moved to mainwindow_menu.cpp
// get_now_selected_list, get_selected_or_group - moved to mainwindow_proxy_list.cpp

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            // take over by shortcut_esc
            break;
        case Qt::Key_Enter:
            // Log before starting
            {
                QString logPath = QDir::currentPath() + "/crash_log.txt";
                QFile logFile(logPath);
                if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                    QTextStream out(&logFile);
                    out << QDateTime::currentDateTime().toString(Qt::ISODate) << " - Key_Enter pressed, calling neko_start()\n";
                    logFile.close();
                }
            }
            neko_start();
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

// Log handling - moved to mainwindow_log.cpp

// eventFilter

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (obj == ui->label_running && mouseEvent->button() == Qt::LeftButton && running != nullptr) {
            speedtest_current();
            return true;
        } else if (obj == ui->label_inbound && mouseEvent->button() == Qt::LeftButton) {
            on_menu_basic_settings_triggered();
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        if (obj == ui->splitter) {
            auto size = ui->splitter->size();
            ui->splitter->setSizes({size.height() / 2, size.height() / 2});
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Profile selector - moved to mainwindow_proxy_list.cpp
// Connection list - moved to mainwindow_connection.cpp
// Hotkey handling - moved to mainwindow_hotkey.cpp
// VPN process management - moved to mainwindow_vpn.cpp
