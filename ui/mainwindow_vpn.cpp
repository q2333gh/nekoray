#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include "db/ConfigBuilder.hpp"
#include "3rdparty/qv2ray/v2/components/proxy/QvProxyConfigurator.hpp"

#ifdef Q_OS_WIN
#include "3rdparty/WinCommander.hpp"
#else
#ifdef Q_OS_LINUX
#include "sys/linux/LinuxCap.h"
#endif
#include <unistd.h>
#endif

#include <QApplication>
#include <QMessageBox>
#include <QProcess>

#define neko_set_spmode_FAILED \
    refresh_status();          \
    return;

void MainWindow::neko_set_spmode_system_proxy(bool enable, bool save) {
    if (enable != NekoGui::dataStore->spmode_system_proxy) {
        if (enable) {
            auto socks_port = NekoGui::dataStore->inbound_socks_port;
            auto http_port = NekoGui::dataStore->inbound_socks_port;
            SetSystemProxy(http_port, socks_port);
        } else {
            ClearSystemProxy();
        }
    }

    if (save) {
        NekoGui::dataStore->remember_spmode.removeAll("system_proxy");
        if (enable && NekoGui::dataStore->remember_enable) {
            NekoGui::dataStore->remember_spmode.append("system_proxy");
        }
        NekoGui::dataStore->Save();
    }

    NekoGui::dataStore->spmode_system_proxy = enable;
    refresh_status();
}

void MainWindow::neko_set_spmode_vpn(bool enable, bool save) {
    if (enable != NekoGui::dataStore->spmode_vpn) {
        if (enable) {
            if (NekoGui::dataStore->vpn_internal_tun) {
                bool requestPermission = !NekoGui::IsAdmin();
                if (requestPermission) {
#ifdef Q_OS_LINUX
                    if (!Linux_HavePkexec()) {
                        MessageBoxWarning(software_name, "Please install \"pkexec\" first.");
                        neko_set_spmode_FAILED
                    }
                    auto ret = Linux_Pkexec_SetCapString(NekoGui::FindNekoBoxCoreRealPath(), "cap_net_admin=ep");
                    if (ret == 0) {
                        this->exit_reason = 3;
                        on_menu_exit_triggered();
                    } else {
                        MessageBoxWarning(software_name, "Setcap for Tun mode failed.\n\n1. You may canceled the dialog.\n2. You may be using an incompatible environment like AppImage.");
                        if (QProcessEnvironment::systemEnvironment().contains("APPIMAGE")) {
                            MW_show_log("If you are using AppImage, it's impossible to start a Tun. Please use other package instead.");
                        }
                    }
#endif
#ifdef Q_OS_WIN
                    auto n = QMessageBox::warning(GetMessageBoxParent(), software_name, tr("Please run NekoBox as admin"), QMessageBox::Yes | QMessageBox::No);
                    if (n == QMessageBox::Yes) {
                        this->exit_reason = 3;
                        on_menu_exit_triggered();
                    }
#endif
                    neko_set_spmode_FAILED
                }
            } else {
                if (NekoGui::dataStore->need_keep_vpn_off) {
                    MessageBoxWarning(software_name, tr("Current server is incompatible with Tun. Please stop the server first, enable Tun Mode, and then restart."));
                    neko_set_spmode_FAILED
                }
                if (!StartVPNProcess()) {
                    neko_set_spmode_FAILED
                }
            }
        } else {
            if (NekoGui::dataStore->vpn_internal_tun) {
                // current core is sing-box
            } else {
                if (!StopVPNProcess()) {
                    neko_set_spmode_FAILED
                }
            }
        }
    }

    if (save) {
        NekoGui::dataStore->remember_spmode.removeAll("vpn");
        if (enable && NekoGui::dataStore->remember_enable) {
            NekoGui::dataStore->remember_spmode.append("vpn");
        }
        NekoGui::dataStore->Save();
    }

    NekoGui::dataStore->spmode_vpn = enable;
    refresh_status();

    if (NekoGui::dataStore->vpn_internal_tun && NekoGui::dataStore->started_id >= 0) neko_start(NekoGui::dataStore->started_id);
}

bool MainWindow::StartVPNProcess() {
    if (vpn_pid != 0) {
        return true;
    }
    auto configPath = NekoGui::WriteVPNSingBoxConfig();
    auto scriptPath = NekoGui::WriteVPNLinuxScript(configPath);
#ifdef Q_OS_WIN
    runOnNewThread([=] {
        // runProcessElevated is a blocking call that does not return the PID.
        // Use sentinel -1 to indicate "VPN process running, PID unknown".
        vpn_pid = -1;
        WinCommander::runProcessElevated(QApplication::applicationDirPath() + "/nekobox_core.exe",
                                         {"--disable-color", "run", "-c", configPath}, "",
                                         NekoGui::dataStore->vpn_hide_console ? WinCommander::SW_HIDE : WinCommander::SW_SHOWMINIMIZED); // blocking
        vpn_pid = 0;
        runOnUiThread([=] { neko_set_spmode_vpn(false); });
    });
#else
    auto vpn_process = new QProcess;
    QProcess::connect(vpn_process, &QProcess::stateChanged, this, [=](QProcess::ProcessState state) {
        if (state == QProcess::NotRunning) {
            vpn_pid = 0;
            vpn_process->deleteLater();
            GetMainWindow()->neko_set_spmode_vpn(false);
        }
    });
    vpn_process->setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef Q_OS_MACOS
    vpn_process->start("osascript", {"-e", QStringLiteral("do shell script \"%1\" with administrator privileges")
                                               .arg("bash " + scriptPath)});
#else
    vpn_process->start("pkexec", {"bash", scriptPath});
#endif
    vpn_process->waitForStarted();
    vpn_pid = vpn_process->processId(); // actually it's pkexec or bash PID
#endif
    return true;
}

bool MainWindow::StopVPNProcess(bool unconditional) {
    if (unconditional || vpn_pid != 0) {
        bool ok;
        core_process->processId();
#ifdef Q_OS_WIN
        auto ret = WinCommander::runProcessElevated("taskkill", {"/IM", "nekobox_core.exe",
                                                                 "/FI",
                                                                 "PID ne " + Int2String(core_process->processId())});
        ok = ret == 0;
#else
        QProcess p;
#ifdef Q_OS_MACOS
        p.start("osascript", {"-e", QStringLiteral("do shell script \"%1\" with administrator privileges")
                                        .arg("pkill -2 -U 0 nekobox_core")});
#else
        if (unconditional) {
            p.start("pkexec", {"killall", "-2", "nekobox_core"});
        } else {
            p.start("pkexec", {"pkill", "-2", "-P", Int2String(vpn_pid)});
        }
#endif
        p.waitForFinished();
        ok = p.exitCode() == 0;
#endif
        if (!unconditional) {
            ok ? vpn_pid = 0 : MessageBoxWarning(tr("Error"), tr("Failed to stop Tun process"));
        }
        return ok;
    }
    return true;
}
