#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include "db/Database.hpp"
#include "db/ConfigBuilder.hpp"
#include "db/traffic/TrafficLooper.hpp"
#include "rpc/gRPC.h"
#include "ui/widget/MessageBoxTimer.h"

#include <QTimer>
#include <QThread>
#include <QInputDialog>
#include <QPushButton>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

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

// Helper function to download core executable
static bool DownloadCoreExecutable(const QString &destDir) {
    WriteCrashLog(QString("Attempting to download core executable to: %1").arg(destDir));
    
#ifdef Q_OS_WIN
    // Find PowerShell script
    QString repoRoot = QDir(QApplication::applicationDirPath()).absolutePath();
    // Try to find repo root by looking for libs/download_core.ps1
    QString scriptPath = repoRoot + "/libs/download_core.ps1";
    QFileInfo scriptInfo(scriptPath);
    
    // If not found in application dir, try going up directories
    if (!scriptInfo.exists()) {
        QDir dir(repoRoot);
        for (int i = 0; i < 5 && !scriptInfo.exists(); i++) {
            dir.cdUp();
            scriptPath = dir.absolutePath() + "/libs/download_core.ps1";
            scriptInfo.setFile(scriptPath);
        }
    }
    
    if (!scriptInfo.exists()) {
        WriteCrashLog(QString("CRITICAL: download_core.ps1 not found. Tried: %1").arg(scriptPath));
        return false;
    }
    
    WriteCrashLog(QString("Found download script at: %1").arg(scriptPath));
    
    // Execute PowerShell script
    QProcess process;
    process.setProgram("powershell.exe");
    QStringList args;
    args << "-ExecutionPolicy" << "Bypass";
    args << "-File" << scriptPath;
    args << "-DestDir" << destDir;
    args << "-Version" << "latest";
    
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    WriteCrashLog("Starting PowerShell download process...");
    process.start();
    
    if (!process.waitForStarted(5000)) {
        WriteCrashLog("CRITICAL: Failed to start PowerShell process");
        return false;
    }
    
    // Wait for completion with timeout (5 minutes)
    if (!process.waitForFinished(300000)) {
        WriteCrashLog("CRITICAL: Download process timed out");
        process.kill();
        return false;
    }
    
    int exitCode = process.exitCode();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    
    WriteCrashLog(QString("Download process finished with exit code: %1").arg(exitCode));
    WriteCrashLog(QString("Output: %1").arg(output));
    
    if (exitCode != 0) {
        WriteCrashLog(QString("CRITICAL: Download failed with exit code: %1").arg(exitCode));
        return false;
    }
    
    // Verify the file was downloaded
    QString corePath = destDir + "/nekobox_core.exe";
    QFileInfo coreInfo(corePath);
    if (!coreInfo.exists() || !coreInfo.isFile()) {
        WriteCrashLog(QString("CRITICAL: Downloaded core file not found at: %1").arg(corePath));
        return false;
    }
    
    WriteCrashLog(QString("Core downloaded successfully to: %1").arg(corePath));
    return true;
#else
    // Linux/Mac: not implemented yet
    WriteCrashLog("Core auto-download not implemented for this platform");
    return false;
#endif
}

// ext core

std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> CreateExtCFromExtR(const std::list<std::shared_ptr<NekoGui_fmt::ExternalBuildResult>> &extRs, bool start) {
    // plz run and start in same thread
    std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> l;
    for (const auto &extR: extRs) {
        std::shared_ptr<NekoGui_sys::ExternalProcess> extC(new NekoGui_sys::ExternalProcess());
        extC->tag = extR->tag;
        extC->program = extR->program;
        extC->arguments = extR->arguments;
        extC->env = extR->env;
        l.emplace_back(extC);
        //
        if (start) extC->Start();
    }
    return l;
}

// grpc

#ifndef NKR_NO_GRPC
using namespace NekoGui_rpc;
#endif

void MainWindow::setup_grpc() {
#ifndef NKR_NO_GRPC
    // Setup Connection
    defaultClient = new Client(
        [=](const QString &errStr) {
            MW_show_log("[Error] gRPC: " + errStr);
        },
        "127.0.0.1:" + Int2String(NekoGui::dataStore->core_port), NekoGui::dataStore->core_token);

    // Looper
    runOnNewThread([=] { NekoGui_traffic::trafficLooper->Loop(); });
#endif
}

// 测速

inline bool speedtesting = false;
inline QList<QThread *> speedtesting_threads = {};

void MainWindow::speedtest_current_group(int mode, bool test_group) {
    WriteCrashLog(QString("speedtest_current_group called: mode=%1, test_group=%2").arg(mode).arg(test_group ? "true" : "false"));
    

          
    if (speedtesting) {
        WriteCrashLog("WARNING: speedtesting already in progress");
        MessageBoxWarning(software_name, QObject::tr("The last speed test did not exit completely, please wait. If it persists, please restart the program."));
        return;
    }

    auto profiles = get_selected_or_group();
    WriteCrashLog(QString("Initial profiles from get_selected_or_group: %1").arg(profiles.size()));
    if (test_group) {
        auto currentGroup = NekoGui::profileManager->CurrentGroup();
        if (currentGroup) {
            profiles = currentGroup->ProfilesWithOrder();
            WriteCrashLog(QString("Using group profiles, count: %1").arg(profiles.size()));
        } else {
            WriteCrashLog("ERROR: CurrentGroup is nullptr when test_group=true");
            return;
        }
    }
    WriteCrashLog(QString("Final profiles count: %1").arg(profiles.size()));
    if (profiles.isEmpty()) {
        WriteCrashLog("ERROR: profiles is empty, returning");
        return;
    }
    auto group = NekoGui::profileManager->CurrentGroup();
    if (!group) {
        WriteCrashLog("ERROR: CurrentGroup is nullptr");
        return;
    }
    if (group->archive) {
        WriteCrashLog("WARNING: group is archived, returning");
        return;
    }
    WriteCrashLog("All checks passed, proceeding with speedtest setup");

    // menu_stop_testing
    if (mode == 114514) {
        WriteCrashLog("Mode is 114514 (stop testing), returning");
        while (!speedtesting_threads.isEmpty()) {
            auto t = speedtesting_threads.takeFirst();
            if (t != nullptr) t->exit();
        }
        speedtesting = false;
        return;
    }

#ifndef NKR_NO_GRPC
    WriteCrashLog("NKR_NO_GRPC is not defined, proceeding with gRPC test");
    QStringList full_test_flags;
    if (mode == libcore::FullTest) {
        auto w = new QDialog(this);
        auto layout = new QVBoxLayout(w);
        w->setWindowTitle(tr("Test Options"));
        //
        auto l1 = new QCheckBox(tr("Latency"));
        auto l2 = new QCheckBox(tr("UDP latency"));
        auto l3 = new QCheckBox(tr("Download speed"));
        auto l4 = new QCheckBox(tr("In and Out IP"));
        //
        auto box = new QDialogButtonBox;
        box->setOrientation(Qt::Horizontal);
        box->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        connect(box, &QDialogButtonBox::accepted, w, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, w, &QDialog::reject);
        //
        layout->addWidget(l1);
        layout->addWidget(l2);
        layout->addWidget(l3);
        layout->addWidget(l4);
        layout->addWidget(box);
        if (w->exec() != QDialog::Accepted) {
            w->deleteLater();
            return;
        }
        //
        if (l1->isChecked()) full_test_flags << "1";
        if (l2->isChecked()) full_test_flags << "2";
        if (l3->isChecked()) full_test_flags << "3";
        if (l4->isChecked()) full_test_flags << "4";
        //
        w->deleteLater();
        if (full_test_flags.isEmpty()) return;
    }
    WriteCrashLog("About to set speedtesting=true and start test thread");
    speedtesting = true;
    WriteCrashLog(QString("speedtesting set to true, starting test thread with %1 profiles, mode=%2").arg(profiles.size()).arg(mode));

    runOnNewThread([this, profiles, mode, full_test_flags]() {
        WriteCrashLog(QString("Test thread started! Processing %1 profiles, mode=%2").arg(profiles.size()).arg(mode));
        QMutex lock_write;
        QMutex lock_return;
        int threadN = NekoGui::dataStore->test_concurrent;
        int threadN_finished = 0;
        auto profiles_test = profiles; // copy

        // Threads
        WriteCrashLog(QString("[THREAD] Creating %1 test worker threads, threadN=%2").arg(threadN).arg(threadN));
        lock_return.lock();
        for (int i = 0; i < threadN; i++) {
            runOnNewThread([&, i] {
                WriteCrashLog(QString("[WORKER] Test worker thread %1 started").arg(i));
                speedtesting_threads << QObject::thread();

                forever {
                    //
                    lock_write.lock();
                    if (profiles_test.isEmpty()) {
                        threadN_finished++;
                        if (threadN == threadN_finished) {
                            // quit control thread
                            lock_return.unlock();
                        }
                        lock_write.unlock();
                        // quit of this thread
                        speedtesting_threads.removeAll(QObject::thread());
                        return;
                    }
                    auto profile = profiles_test.takeFirst();
                    lock_write.unlock();

                    //
                    libcore::TestReq req;
                    req.set_mode((libcore::TestMode) mode);
                    req.set_timeout(10 * 1000);
                    req.set_url(NekoGui::dataStore->test_latency_url.toStdString());

                    //
                    std::list<std::shared_ptr<NekoGui_sys::ExternalProcess>> extCs;
                    QSemaphore extSem;

                    if (mode == libcore::TestMode::UrlTest || mode == libcore::FullTest) {
                        auto c = BuildConfig(profile, true, false);
                        if (!c->error.isEmpty()) {
                            profile->full_test_report = c->error;
                            profile->Save();
                            auto profileId = profile->id;
                            runOnUiThread([this, profileId] {
                                refresh_proxy_list(profileId);
                            });
                            continue;
                        }
                        //
                        if (!c->extRs.empty()) {
                            runOnUiThread(
                                [&] {
                                    extCs = CreateExtCFromExtR(c->extRs, true);
                                    QThread::msleep(500);
                                    extSem.release();
                                },
                                DS_cores);
                            extSem.acquire();
                        }
                        //
                        auto config = new libcore::LoadConfigReq;
                        config->set_core_config(QJsonObject2QString(c->coreConfig, false).toStdString());
                        req.set_allocated_config(config);
                        req.set_in_address(profile->bean->serverAddress.toStdString());

                        req.set_full_latency(full_test_flags.contains("1"));
                        req.set_full_udp_latency(full_test_flags.contains("2"));
                        req.set_full_speed(full_test_flags.contains("3"));
                        req.set_full_in_out(full_test_flags.contains("4"));

                        req.set_full_speed_url(NekoGui::dataStore->test_download_url.toStdString());
                        req.set_full_speed_timeout(NekoGui::dataStore->test_download_timeout);
                    } else if (mode == libcore::TcpPing) {
                        req.set_address(profile->bean->DisplayAddress().toStdString());
                    }

                    bool rpcOK;
                    if (defaultClient == nullptr) {
                        WriteCrashLog(QString("[Test] ERROR: gRPC client not available for profile: %1").arg(profile->bean->DisplayTypeAndName()));
                        MW_show_log(QString("[Test] gRPC client not available for profile: %1").arg(profile->bean->DisplayTypeAndName()));
                        continue;
                    }
                    WriteCrashLog(QString("[Test] Starting test for profile: %1, mode=%2").arg(profile->bean->DisplayTypeAndName()).arg(mode));
                    MW_show_log(QString("[Test] Testing profile: %1").arg(profile->bean->DisplayTypeAndName()));
                    auto result = defaultClient->Test(&rpcOK, req);
                    WriteCrashLog(QString("[Test] gRPC Test completed: rpcOK=%1").arg(rpcOK ? "true" : "false"));
                    if (!rpcOK) {
                        WriteCrashLog(QString("[Test] ERROR: gRPC call failed for profile: %1").arg(profile->bean->DisplayTypeAndName()));
                        MW_show_log(QString("[Test] gRPC call failed for profile: %1").arg(profile->bean->DisplayTypeAndName()));
                    } else {
                        WriteCrashLog(QString("[Test] SUCCESS: Profile %1 tested successfully").arg(profile->bean->DisplayTypeAndName()));
                        WriteCrashLog(QString("[Test] Result: latency=%1 ms, error='%2'").arg(result.ms()).arg(result.error().c_str()));
                        if (result.ms() > 0) {
                            WriteCrashLog(QString("[Test] FINAL: URL test succeeded with latency %1 ms for %2").arg(result.ms()).arg(profile->bean->DisplayTypeAndName()));
                        }
                    }
                    //
                    if (!extCs.empty()) {
                        runOnUiThread(
                            [&] {
                                for (const auto &extC: extCs) {
                                    extC->Kill();
                                }
                                extSem.release();
                            },
                            DS_cores);
                        extSem.acquire();
                    }
                    //
                    if (!rpcOK) return;

                    if (result.error().empty()) {
                        profile->latency = result.ms();
                        if (profile->latency == 0) profile->latency = 1; // nekoray use 0 to represents not tested
                    } else {
                        profile->latency = -1;
                    }
                    profile->full_test_report = result.full_report().c_str(); // higher priority
                    profile->Save();

                    if (!result.error().empty()) {
                        MW_show_log(tr("[%1] test error: %2").arg(profile->bean->DisplayTypeAndName(), result.error().c_str()));
                    }

                    auto profileId = profile->id;
                    runOnUiThread([this, profileId] {
                        refresh_proxy_list(profileId);
                    });
                }
            });
        }

        // Control
        lock_return.lock();
        lock_return.unlock();
        speedtesting = false;
        MW_show_log(QObject::tr("Speedtest finished."));
    });
#endif
}

void MainWindow::speedtest_current() {
#ifndef NKR_NO_GRPC
    WriteCrashLog("speedtest_current called");
    
    // Check if defaultClient is available
    if (defaultClient == nullptr) {
        WriteCrashLog("CRITICAL: defaultClient is nullptr in speedtest_current");
        MW_show_log("[Error] gRPC client is not initialized. Cannot perform URL test.");
        MessageBoxWarning(software_name, "gRPC client is not initialized. Please ensure core is running.");
        return;
    }
    WriteCrashLog("defaultClient is valid");
    
    // Check if core is running
    if (!NekoGui::dataStore->core_running) {
        WriteCrashLog("WARNING: core_running is false");
        MW_show_log("[Error] Core is not running. Cannot perform URL test.");
        MessageBoxWarning(software_name, "Core is not running. Please start a profile first.");
        return;
    }
    WriteCrashLog("core_running is true");
    
    last_test_time = QTime::currentTime();
    ui->label_running->setText(tr("Testing"));
    MW_show_log(QString("[URL Test] Starting test to: %1").arg(NekoGui::dataStore->test_latency_url));
    WriteCrashLog(QString("Starting URL test to: %1").arg(NekoGui::dataStore->test_latency_url));

    runOnNewThread([=] {
        WriteCrashLog("URL test thread started");
        libcore::TestReq req;
        req.set_mode(libcore::UrlTest);
        req.set_timeout(10 * 1000);
        req.set_url(NekoGui::dataStore->test_latency_url.toStdString());

        bool rpcOK;
        MW_show_log("[URL Test] Calling gRPC Test...");
        WriteCrashLog("Calling defaultClient->Test...");
        auto result = defaultClient->Test(&rpcOK, req);
        WriteCrashLog(QString("Test returned: rpcOK=%1").arg(rpcOK ? "true" : "false"));
        if (!rpcOK) {
            MW_show_log("[URL Test] gRPC call failed");
            WriteCrashLog("gRPC Test call failed");
            return;
        }
        WriteCrashLog(QString("Test succeeded, latency: %1 ms").arg(result.ms()));
        MW_show_log(QString("[URL Test] gRPC call succeeded, latency: %1 ms").arg(result.ms()));

        auto latency = result.ms();
        last_test_time = QTime::currentTime();

        runOnUiThread([=] {
            if (!result.error().empty()) {
                MW_show_log(QStringLiteral("UrlTest error: %1").arg(result.error().c_str()));
            }
            if (latency <= 0) {
                ui->label_running->setText(tr("Test Result") + ": " + tr("Unavailable"));
            } else if (latency > 0) {
                ui->label_running->setText(tr("Test Result") + ": " + QStringLiteral("%1 ms").arg(latency));
            }
        });
    });
#endif
}

void MainWindow::stop_core_daemon() {
#ifndef NKR_NO_GRPC
    NekoGui_rpc::defaultClient->Exit();
#endif
}

void MainWindow::neko_start(int _id) {
    WriteCrashLog(QString("neko_start called with _id=%1").arg(_id));
    
    if (NekoGui::dataStore->prepare_exit) {
        WriteCrashLog("prepare_exit is true, returning");
        return;
    }

    // Check core process first, create if not exists
    if (core_process == nullptr) {
        WriteCrashLog("WARNING: core_process is nullptr, attempting to initialize...");
        
        // Try to find core executable
        QString core_path = QApplication::applicationDirPath() + "/";
#ifdef Q_OS_WIN
        core_path += "nekobox_core.exe";
#else
        core_path += "nekobox_core";
#endif
        
        // Try alternative paths if not found
        QFileInfo coreFileInfo(core_path);
        if (!coreFileInfo.exists() || !coreFileInfo.isFile()) {
            // Try current working directory
            QString cwd_core_path = QDir::currentPath() + "/";
#ifdef Q_OS_WIN
            cwd_core_path += "nekobox_core.exe";
#else
            cwd_core_path += "nekobox_core";
#endif
            QFileInfo cwdCoreFileInfo(cwd_core_path);
            if (cwdCoreFileInfo.exists() && cwdCoreFileInfo.isFile()) {
                core_path = cwd_core_path;
                WriteCrashLog(QString("Found core at current working directory: %1").arg(core_path));
            } else {
                // Core not found, try to download it automatically
                WriteCrashLog(QString("Core executable not found, attempting automatic download..."));
                WriteCrashLog(QString("Tried paths:"));
                WriteCrashLog(QString("  1. %1").arg(core_path));
                WriteCrashLog(QString("  2. %1").arg(cwd_core_path));
                
                // Show message to user
                runOnUiThread([=] {
                    MessageBoxInfo(software_name, QString("Core executable not found.\n\nAttempting to download automatically...\n\nThis may take a few minutes."));
                });
                
                // Try to download to application directory
                QString destDir = QApplication::applicationDirPath();
                if (DownloadCoreExecutable(destDir)) {
                    // Verify the downloaded file
                    QString downloadedPath = destDir + "/nekobox_core.exe";
                    QFileInfo downloadedInfo(downloadedPath);
                    if (downloadedInfo.exists() && downloadedInfo.isFile()) {
                        core_path = downloadedPath;
                        WriteCrashLog(QString("Core downloaded successfully, using: %1").arg(core_path));
                        runOnUiThread([=] {
                            MessageBoxInfo(software_name, QString("Core downloaded successfully!\n\nPath: %1").arg(core_path));
                        });
                    } else {
                        WriteCrashLog(QString("CRITICAL: Downloaded core file verification failed"));
                        runOnUiThread([=] {
                            MessageBoxWarning(software_name, QString("Failed to download core executable.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases\n\nAnd place it in: %1").arg(destDir));
                        });
                        return;
                    }
                } else {
                    WriteCrashLog(QString("CRITICAL: Automatic download failed"));
                    runOnUiThread([=] {
                        MessageBoxWarning(software_name, QString("Failed to download core executable automatically.\n\nPlease manually download nekobox_core.exe from:\nhttps://github.com/MatsuriDayo/nekoray/releases\n\nAnd place it in: %1").arg(destDir));
                    });
                    return;
                }
            }
        }
        
        // Initialize core process - must be done in UI thread (DS_cores thread)
        WriteCrashLog(QString("Initializing core_process with path: %1").arg(core_path));
        QString found_core_path = core_path; // Capture for lambda
        
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
        // Use runOnUiThread to ensure thread safety
        WriteCrashLog("Scheduling core_process creation in DS_cores thread...");
        runOnUiThread(
            [=] {
                WriteCrashLog("Creating core_process in DS_cores thread...");
                if (core_process == nullptr) {
                    core_process = new NekoGui_sys::CoreProcess(found_core_path, args);
                    WriteCrashLog("core_process created successfully");
                    
                    // Remember last started
                    if (NekoGui::dataStore->remember_enable && NekoGui::dataStore->remember_id >= 0) {
                        core_process->start_profile_when_core_is_up = NekoGui::dataStore->remember_id;
                    }
                    
                    // Start core process
                    core_process->Start();
                    WriteCrashLog("core_process started");
                    
                    // Setup gRPC
                    setup_grpc();
                    WriteCrashLog("gRPC setup completed");
                } else {
                    WriteCrashLog("core_process already exists, skipping creation");
                }
            },
            DS_cores);
        
        // Wait a bit for initialization to complete
        // Note: runOnUiThread is asynchronous, so we wait a bit
        WriteCrashLog("Waiting for core_process initialization...");
        QThread::msleep(1000);
        
        // Check if core_process was created
        if (core_process == nullptr) {
            WriteCrashLog("CRITICAL: core_process is still nullptr after initialization attempt");
            WriteCrashLog("This may indicate a threading issue. Please restart the application.");
            runOnUiThread([=] {
                MessageBoxWarning(software_name, "Failed to initialize core process. Please restart the application.");
            });
            return;
        }
        
        WriteCrashLog("core_process initialization completed successfully");
    }
    WriteCrashLog("core_process is valid");

    // Get core path from core_process (already validated in constructor)
    QString core_path = core_process->program;
    WriteCrashLog(QString("Using core path from core_process: %1").arg(core_path));
    
    // Verify the path still exists (in case it was deleted after startup)
    QFileInfo coreFileInfo(core_path);
    if (!coreFileInfo.exists() || !coreFileInfo.isFile()) {
        WriteCrashLog(QString("WARNING: Core executable not found at stored path: %1").arg(core_path));
        WriteCrashLog(QString("Application dir: %1").arg(QApplication::applicationDirPath()));
        WriteCrashLog(QString("Current dir: %1").arg(QDir::currentPath()));
        
        // Try to find core in application directory (same logic as constructor)
        QString alt_core_path = QApplication::applicationDirPath() + "/";
#ifdef Q_OS_WIN
        alt_core_path += "nekobox_core.exe";
#else
        alt_core_path += "nekobox_core";
#endif
        QFileInfo altCoreFileInfo(alt_core_path);
        if (altCoreFileInfo.exists() && altCoreFileInfo.isFile()) {
            WriteCrashLog(QString("Found core at alternative path: %1").arg(alt_core_path));
            core_path = alt_core_path;
            // Update core_process with the correct path
            core_process->program = core_path;
        } else {
            // Try current working directory
            QString cwd_core_path = QDir::currentPath() + "/";
#ifdef Q_OS_WIN
            cwd_core_path += "nekobox_core.exe";
#else
            cwd_core_path += "nekobox_core";
#endif
            QFileInfo cwdCoreFileInfo(cwd_core_path);
            if (cwdCoreFileInfo.exists() && cwdCoreFileInfo.isFile()) {
                WriteCrashLog(QString("Found core at current working directory: %1").arg(cwd_core_path));
                core_path = cwd_core_path;
                core_process->program = core_path;
            } else {
                WriteCrashLog(QString("CRITICAL: Core executable not found at any location"));
                WriteCrashLog(QString("Tried paths:"));
                WriteCrashLog(QString("  1. %1").arg(core_path));
                WriteCrashLog(QString("  2. %1").arg(alt_core_path));
                WriteCrashLog(QString("  3. %1").arg(cwd_core_path));
                runOnUiThread([=] {
                    MessageBoxWarning(software_name, QString("Core executable not found!\n\nTried paths:\n1. %1\n2. %2\n3. %3\n\nPlease ensure nekobox_core is present.").arg(core_path, alt_core_path, cwd_core_path));
                });
                return;
            }
        }
    }
    WriteCrashLog(QString("Core executable verified at: %1").arg(core_path));

    // Check gRPC client
#ifndef NKR_NO_GRPC
    if (defaultClient == nullptr) {
        WriteCrashLog("CRITICAL: defaultClient is nullptr");
        WriteCrashLog(QString("core_running=%1").arg(NekoGui::dataStore->core_running ? "true" : "false"));
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
        //
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
        //
        WriteCrashLog("Setting up traffic looper...");
        NekoGui_traffic::trafficLooper->proxy = result->outboundStat.get();
        NekoGui_traffic::trafficLooper->items = result->outboundStats;
        NekoGui::dataStore->ignoreConnTag = result->ignoreConnTag;
        NekoGui_traffic::trafficLooper->loop_enabled = true;
        WriteCrashLog("Traffic looper setup complete");
#endif

        runOnUiThread(
            [=] {
                auto extCs = CreateExtCFromExtR(result->extRs, true);
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
        return; // let CoreProcess call neko_start when core is up
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
            // Auto-trigger URL test after profile starts successfully (for automated testing)
            // Wait a bit for the profile to fully establish connection
            QThread::msleep(2000);
            WriteCrashLog("Auto-triggering URL test after profile start...");
            runOnUiThread([=] {
                speedtest_current_group(1, true);  // mode=1 (URL test), test_group=true
            });
        }
        mu_starting.unlock();
        // cancel timeout
        runOnUiThread([=] {
            restartMsgboxTimer->cancel();
            restartMsgboxTimer->deleteLater();
            restartMsgbox->deleteLater();
#ifdef Q_OS_LINUX
            // Check systemd-resolved
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

void MainWindow::CheckUpdate() {
    // on new thread...
#ifndef NKR_NO_GRPC
    bool ok;
    libcore::UpdateReq request;
    request.set_action(libcore::UpdateAction::Check);
    request.set_check_pre_release(NekoGui::dataStore->check_include_pre);
    auto response = NekoGui_rpc::defaultClient->Update(&ok, request);
    if (!ok) return;

    auto err = response.error();
    if (!err.empty()) {
        runOnUiThread([=] {
            MessageBoxWarning(QObject::tr("Update"), err.c_str());
        });
        return;
    }

    if (response.release_download_url() == nullptr) {
        runOnUiThread([=] {
            MessageBoxInfo(QObject::tr("Update"), QObject::tr("No update"));
        });
        return;
    }

    runOnUiThread([=] {
        auto allow_updater = !NekoGui::dataStore->flag_use_appdata;
        auto note_pre_release = response.is_pre_release() ? " (Pre-release)" : "";
        QMessageBox box(QMessageBox::Question, QObject::tr("Update") + note_pre_release,
                        QObject::tr("Update found: %1\nRelease note:\n%2").arg(response.assets_name().c_str(), response.release_note().c_str()));
        //
        QAbstractButton *btn1 = nullptr;
        if (allow_updater) {
            btn1 = box.addButton(QObject::tr("Update"), QMessageBox::AcceptRole);
        }
        QAbstractButton *btn2 = box.addButton(QObject::tr("Open in browser"), QMessageBox::AcceptRole);
        box.addButton(QObject::tr("Close"), QMessageBox::RejectRole);
        box.exec();
        //
        if (btn1 == box.clickedButton() && allow_updater) {
            // Download Update
            runOnNewThread([=] {
                bool ok2;
                libcore::UpdateReq request2;
                request2.set_action(libcore::UpdateAction::Download);
                auto response2 = NekoGui_rpc::defaultClient->Update(&ok2, request2);
                runOnUiThread([=] {
                    if (response2.error().empty()) {
                        auto q = QMessageBox::question(nullptr, QObject::tr("Update"),
                                                       QObject::tr("Update is ready, restart to install?"));
                        if (q == QMessageBox::StandardButton::Yes) {
                            this->exit_reason = 1;
                            on_menu_exit_triggered();
                        }
                    } else {
                        MessageBoxWarning(QObject::tr("Update"), response2.error().c_str());
                    }
                });
            });
        } else if (btn2 == box.clickedButton()) {
            QDesktopServices::openUrl(QUrl(response.release_url().c_str()));
        }
    });
#endif
}
