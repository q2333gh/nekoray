#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include "db/Database.hpp"
#include "db/ConfigBuilder.hpp"
#include "db/traffic/TrafficLooper.hpp"
#include "rpc/gRPC.h"
#include "ui/widget/MessageBoxTimer.h"

#include <QThread>
#include <QInputDialog>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

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

// ext core - shared helper function
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
                        auto c = NekoGui::BuildConfig(profile, true, false);
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
