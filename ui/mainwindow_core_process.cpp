#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include "mainwindow_core_download.h"

#include "sys/ExternalProcess.hpp"
#include "fmt/AbstractBean.hpp"

#include "db/ConfigBuilder.hpp"
#include "db/traffic/TrafficLooper.hpp"
#include "rpc/gRPC.h"
#include "sys/ExternalProcess.hpp"
#include "ui/widget/MessageBoxTimer.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <QThread>
#include <QMessageBox>

// Forward declaration
extern bool DownloadCoreExecutable(const QString &destDir);

// Helper function to write crash log
static void WriteCrashLog(const QString &message) {
    QString logPath = QDir::currentPath() + "/crash_log.txt";
    QFile logFile(logPath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << " - " << message << "\n";
        logFile.close();
    }
}

// Helper to find core executable path
static QString FindCoreExecutablePath() {
    QString core_path = QApplication::applicationDirPath() + "/";
#ifdef Q_OS_WIN
    core_path += "nekobox_core.exe";
#else
    core_path += "nekobox_core";
#endif
    
    QFileInfo coreFileInfo(core_path);
    if (coreFileInfo.exists() && coreFileInfo.isFile()) {
        return core_path;
    }
    
    // Try current working directory
    QString cwd_core_path = QDir::currentPath() + "/";
#ifdef Q_OS_WIN
    cwd_core_path += "nekobox_core.exe";
#else
    cwd_core_path += "nekobox_core";
#endif
    QFileInfo cwdCoreFileInfo(cwd_core_path);
    if (cwdCoreFileInfo.exists() && cwdCoreFileInfo.isFile()) {
        return cwd_core_path;
    }
    
    return QString();
}

// Helper to initialize core process
static bool InitializeCoreProcess(MainWindow *mw) {
    QString core_path = FindCoreExecutablePath();
    
    if (core_path.isEmpty()) {
        WriteCrashLog("Core executable not found, attempting automatic download...");
        runOnUiThread([=] {
            MessageBoxInfo(software_name, QString("Core executable not found.\n\nAttempting to download automatically...\n\nThis may take a few minutes."));
        });
        
        QString destDir = QApplication::applicationDirPath();
        if (!DownloadCoreExecutable(destDir)) {
            WriteCrashLog("CRITICAL: Automatic download failed");
            runOnUiThread([=] {
                MessageBoxWarning(software_name, QString("Failed to download core executable automatically.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases\n\nAnd place it in: %1").arg(destDir));
            });
            return false;
        }
        
        // Verify downloaded file
        QString downloadedPath = destDir + "/nekobox_core.exe";
        QFileInfo downloadedInfo(downloadedPath);
        if (downloadedInfo.exists() && downloadedInfo.isFile()) {
            core_path = downloadedPath;
            WriteCrashLog(QString("Core downloaded successfully, using: %1").arg(core_path));
            runOnUiThread([=] {
                MessageBoxInfo(software_name, QString("Core downloaded successfully!\n\nPath: %1").arg(core_path));
            });
        } else {
            WriteCrashLog("CRITICAL: Downloaded core file verification failed");
            runOnUiThread([=] {
                MessageBoxWarning(software_name, QString("Failed to download core executable.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases\n\nAnd place it in: %1").arg(destDir));
            });
            return false;
        }
    }
    
    // Generate token and port
    NekoGui::dataStore->core_token = GetRandomString(32);
    NekoGui::dataStore->core_port = MkPort();
    if (NekoGui::dataStore->core_port <= 0) NekoGui::dataStore->core_port = 19810;
    
    QStringList args;
    args.push_back("nekobox");
    args.push_back("-port");
    args.push_back(Int2String(NekoGui::dataStore->core_port));
    if (NekoGui::dataStore->flag_debug) args.push_back("-debug");
    
    // Create core process in the correct thread (DS_cores)
    WriteCrashLog(QString("Initializing core_process with path: %1").arg(core_path));
    QString found_core_path = core_path;
    
    runOnUiThread(
        [=] {
            if (mw->core_process == nullptr) {
                mw->core_process = new NekoGui_sys::CoreProcess(found_core_path, args);
                WriteCrashLog("core_process created successfully");
                
                if (NekoGui::dataStore->remember_enable && NekoGui::dataStore->remember_id >= 0) {
                    mw->core_process->start_profile_when_core_is_up = NekoGui::dataStore->remember_id;
                }
                
                mw->core_process->Start();
                WriteCrashLog("core_process started");
                
                mw->setup_grpc();
                WriteCrashLog("gRPC setup completed");
            }
        },
        DS_cores);
    
    QThread::msleep(1000);
    
    if (mw->core_process == nullptr) {
        WriteCrashLog("CRITICAL: core_process is still nullptr after initialization attempt");
        runOnUiThread([=] {
            MessageBoxWarning(software_name, "Failed to initialize core process. Please restart the application.");
        });
        return false;
    }
    
    return true;
}

void MainWindow::neko_start(int _id) {
    WriteCrashLog(QString("neko_start called with _id=%1").arg(_id));
    
    if (NekoGui::dataStore->prepare_exit) {
        WriteCrashLog("prepare_exit is true, returning");
        return;
    }

    // Check core process first, create if not exists
    if (core_process == nullptr) {
        if (!InitializeCoreProcess(this)) {
            return;
        }
    }
    WriteCrashLog("core_process is valid");

    // Verify core path
    QString core_path = core_process->program;
    QFileInfo coreFileInfo(core_path);
    if (!coreFileInfo.exists() || !coreFileInfo.isFile()) {
        QString alt_path = FindCoreExecutablePath();
        if (!alt_path.isEmpty()) {
            core_path = alt_path;
            core_process->program = core_path;
        } else {
            WriteCrashLog("CRITICAL: Core executable not found at any location");
            runOnUiThread([=] {
                MessageBoxWarning(software_name, QString("Core executable not found!\n\nPlease ensure nekobox_core is present."));
            });
            return;
        }
    }
    WriteCrashLog(QString("Core executable verified at: %1").arg(core_path));

#ifndef NKR_NO_GRPC
    if (defaultClient == nullptr) {
        WriteCrashLog("CRITICAL: defaultClient is nullptr");
        runOnUiThread([=] {
            MessageBoxWarning(software_name, "gRPC client is not initialized. Core may not be running. Please restart the application.");
        });
        return;
    }
    WriteCrashLog("defaultClient is valid");
#endif

    auto ents = get_now_selected_list();
    WriteCrashLog(QString("get_now_selected_list returned %1 items").arg(ents.size()));
    
    auto ent = (_id < 0 && !ents.isEmpty()) ? ents.first() : NekoGui::profileManager->GetProfile(_id);
    if (ent == nullptr) {
        WriteCrashLog(QString("ent is nullptr, _id=%1, ents.size()=%2").arg(_id).arg(ents.size()));
        return;
    }
    WriteCrashLog(QString("Selected profile: id=%1, type=%2, name=%3").arg(ent->id).arg(ent->bean->DisplayType()).arg(ent->bean->name));

    if (select_mode) {
        WriteCrashLog("select_mode is true, emitting profile_selected");
        emit profile_selected(ent->id);
        select_mode = false;
        refresh_status();
        return;
    }

    auto group = NekoGui::profileManager->GetGroup(ent->gid);
    if (group == nullptr || group->archive) {
        WriteCrashLog(QString("Group is nullptr or archived, gid=%1").arg(ent->gid));
        return;
    }
    WriteCrashLog(QString("Group is valid: gid=%1, name=%2").arg(group->id).arg(group->name));

    WriteCrashLog("Calling BuildConfig...");
    auto result = BuildConfig(ent, false, false);
    if (!result->error.isEmpty()) {
        WriteCrashLog(QString("BuildConfig error: %1").arg(result->error));
        MessageBoxWarning("BuildConfig return error", result->error);
        return;
    }
    WriteCrashLog("BuildConfig succeeded");

    auto neko_start_stage2 = [=] {
        WriteCrashLog("neko_start_stage2 called");
#ifndef NKR_NO_GRPC
        if (defaultClient == nullptr) {
            WriteCrashLog("CRITICAL: defaultClient is nullptr in neko_start_stage2");
            return false;
        }
        WriteCrashLog("Preparing LoadConfigReq...");
        libcore::LoadConfigReq req;
        req.set_core_config(QJsonObject2QString(result->coreConfig, false).toStdString());
        req.set_enable_nekoray_connections(NekoGui::dataStore->connection_statistics);
        if (NekoGui::dataStore->traffic_loop_interval > 0) {
            req.add_stats_outbounds("proxy");
            req.add_stats_outbounds("bypass");
        }
        
        WriteCrashLog("Calling defaultClient->Start...");
        bool rpcOK;
        QString error = defaultClient->Start(&rpcOK, req);
        WriteCrashLog(QString("defaultClient->Start returned: rpcOK=%1, error=%2").arg(rpcOK ? "true" : "false").arg(error));
        if (rpcOK && !error.isEmpty()) {
            WriteCrashLog(QString("LoadConfig error: %1").arg(error));
            runOnUiThread([=] { MessageBoxWarning("LoadConfig return error", error); });
            return false;
        } else if (!rpcOK) {
            WriteCrashLog("LoadConfig failed: rpcOK is false");
            return false;
        }
        WriteCrashLog("LoadConfig succeeded");
        
        WriteCrashLog("Setting up traffic looper...");
        NekoGui_traffic::trafficLooper->proxy = result->outboundStat.get();
        NekoGui_traffic::trafficLooper->items = result->outboundStats;
        NekoGui::dataStore->ignoreConnTag = result->ignoreConnTag;
        NekoGui_traffic::trafficLooper->loop_enabled = true;
        WriteCrashLog("Traffic looper setup complete");
#endif

        runOnUiThread(
            [=] {
                std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> extCs;
                for (const auto &extR: result->extRs) {
                    std::shared_ptr<NekoGui_sys::ExternalProcess> extC(new NekoGui_sys::ExternalProcess());
                    extC->tag = extR->tag;
                    extC->program = extR->program;
                    extC->arguments = extR->arguments;
                    extC->env = extR->env;
                    extCs.emplace_back(extC);
                    extC->Start();
                }
                NekoGui_sys::running_ext.splice(NekoGui_sys::running_ext.end(), extCs);
            },
            DS_cores);

        NekoGui::dataStore->UpdateStartedId(ent->id);
        running = ent;

        runOnUiThread([=] {
            refresh_status();
            refresh_proxy_list(ent->id);
        });

        return true;
    };

    if (!mu_starting.tryLock()) {
        MessageBoxWarning(software_name, "Another profile is starting...");
        return;
    }
    if (!mu_stopping.tryLock()) {
        MessageBoxWarning(software_name, "Another profile is stopping...");
        mu_starting.unlock();
        return;
    }
    mu_stopping.unlock();

    // check core state
    WriteCrashLog(QString("Checking core state: core_running=%1").arg(NekoGui::dataStore->core_running ? "true" : "false"));
    if (!NekoGui::dataStore->core_running) {
        WriteCrashLog("Core is not running, attempting to restart...");
        if (core_process == nullptr) {
            WriteCrashLog("CRITICAL: core_process is nullptr when trying to restart");
            runOnUiThread([=] {
                MessageBoxWarning(software_name, "Core process is not initialized. Cannot restart core.");
            });
            mu_starting.unlock();
            return;
        }
        runOnUiThread(
            [=] {
                MW_show_log("Try to start the config, but the core has not listened to the grpc port, so restart it...");
                core_process->start_profile_when_core_is_up = ent->id;
                core_process->Restart();
            },
            DS_cores);
        mu_starting.unlock();
        WriteCrashLog("Core restart initiated, returning");
        return;
    }
    WriteCrashLog("Core is running, proceeding with start");

    // timeout message
    auto restartMsgbox = new QMessageBox(QMessageBox::Question, software_name, tr("If there is no response for a long time, it is recommended to restart the software."),
                                         QMessageBox::Yes | QMessageBox::No, this);
    connect(restartMsgbox, &QMessageBox::accepted, this, [=] { MW_dialog_message("", "RestartProgram"); });
    auto restartMsgboxTimer = new MessageBoxTimer(this, restartMsgbox, 5000);

    runOnNewThread([=] {
        WriteCrashLog("Starting profile in new thread...");
        // stop current running
        if (NekoGui::dataStore->started_id >= 0) {
            WriteCrashLog(QString("Stopping current profile: id=%1").arg(NekoGui::dataStore->started_id));
            runOnUiThread([=] { neko_stop(false, true); });
            sem_stopped.acquire();
            WriteCrashLog("Current profile stopped");
        }
        // do start
        WriteCrashLog(QString("Starting profile: %1").arg(ent->bean->DisplayTypeAndName()));
        MW_show_log(">>>>>>>> " + tr("Starting profile %1").arg(ent->bean->DisplayTypeAndName()));
        if (!neko_start_stage2()) {
            WriteCrashLog(QString("Failed to start profile: %1").arg(ent->bean->DisplayTypeAndName()));
            MW_show_log("<<<<<<<< " + tr("Failed to start profile %1").arg(ent->bean->DisplayTypeAndName()));
        } else {
            WriteCrashLog(QString("Profile started successfully: %1").arg(ent->bean->DisplayTypeAndName()));
            QThread::msleep(2000);
            WriteCrashLog("Auto-triggering URL test after profile start...");
            runOnUiThread([=] {
                speedtest_current_group(1, true);
            });
        }
        mu_starting.unlock();
        // cancel timeout
        runOnUiThread([=] {
            restartMsgboxTimer->cancel();
            restartMsgboxTimer->deleteLater();
            restartMsgbox->deleteLater();
#ifdef Q_OS_LINUX
            if (NekoGui::dataStore->spmode_vpn && NekoGui::dataStore->routing->direct_dns.startsWith("local") && ReadFileText("/etc/resolv.conf").contains("systemd-resolved")) {
                MW_show_log("[Warning] The default Direct DNS may not works with systemd-resolved, you may consider change your DNS settings.");
            }
#endif
        });
    });
}

void MainWindow::neko_stop(bool crash, bool sem) {
    auto id = NekoGui::dataStore->started_id;
    if (id < 0) {
        if (sem) sem_stopped.release();
        return;
    }

    auto neko_stop_stage2 = [=] {
        runOnUiThread(
            [=] {
                while (!NekoGui_sys::running_ext.empty()) {
                    auto extC = NekoGui_sys::running_ext.front();
                    extC->Kill();
                    NekoGui_sys::running_ext.pop_front();
                }
            },
            DS_cores);

#ifndef NKR_NO_GRPC
        NekoGui_traffic::trafficLooper->loop_enabled = false;
        NekoGui_traffic::trafficLooper->loop_mutex.lock();
        if (NekoGui::dataStore->traffic_loop_interval != 0) {
            NekoGui_traffic::trafficLooper->UpdateAll();
            for (const auto &item: NekoGui_traffic::trafficLooper->items) {
                NekoGui::profileManager->GetProfile(item->id)->Save();
                runOnUiThread([=] { refresh_proxy_list(item->id); });
            }
        }
        NekoGui_traffic::trafficLooper->loop_mutex.unlock();

        if (!crash) {
            bool rpcOK;
            QString error = defaultClient->Stop(&rpcOK);
            if (rpcOK && !error.isEmpty()) {
                runOnUiThread([=] { MessageBoxWarning("Stop return error", error); });
                return false;
            } else if (!rpcOK) {
                return false;
            }
        }
#endif

        NekoGui::dataStore->UpdateStartedId(-1919);
        NekoGui::dataStore->need_keep_vpn_off = false;
        running = nullptr;

        runOnUiThread([=] {
            refresh_status();
            refresh_proxy_list(id);
        });

        return true;
    };

    if (!mu_stopping.tryLock()) {
        if (sem) sem_stopped.release();
        return;
    }

    // timeout message
    auto restartMsgbox = new QMessageBox(QMessageBox::Question, software_name, tr("If there is no response for a long time, it is recommended to restart the software."),
                                         QMessageBox::Yes | QMessageBox::No, this);
    connect(restartMsgbox, &QMessageBox::accepted, this, [=] { MW_dialog_message("", "RestartProgram"); });
    auto restartMsgboxTimer = new MessageBoxTimer(this, restartMsgbox, 5000);

    runOnNewThread([=] {
        // do stop
        MW_show_log(">>>>>>>> " + tr("Stopping profile %1").arg(running->bean->DisplayTypeAndName()));
        if (!neko_stop_stage2()) {
            MW_show_log("<<<<<<<< " + tr("Failed to stop, please restart the program."));
        }
        mu_stopping.unlock();
        if (sem) sem_stopped.release();
        // cancel timeout
        runOnUiThread([=] {
            restartMsgboxTimer->cancel();
            restartMsgboxTimer->deleteLater();
            restartMsgbox->deleteLater();
        });
    });
}

void MainWindow::stop_core_daemon() {
#ifndef NKR_NO_GRPC
    NekoGui_rpc::defaultClient->Exit();
#endif
}
