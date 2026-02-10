#include "ExternalProcess.hpp"
#include "main/NekoGui.hpp"

#include <QTimer>
#include <QDir>
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

namespace NekoGui_sys {

    ExternalProcess::ExternalProcess() : QProcess() {
        // qDebug() << "[Debug] ExternalProcess()" << this << running_ext;
        this->env = QProcessEnvironment::systemEnvironment().toStringList();
    }

    ExternalProcess::~ExternalProcess() {
        // qDebug() << "[Debug] ~ExternalProcess()" << this << running_ext;
    }

    void ExternalProcess::Start() {
        if (started) return;
        started = true;

        if (managed) {
            connect(this, &QProcess::readyReadStandardOutput, this, [this]() {
                auto log = readAllStandardOutput();
                if (logCounter.fetchAndAddRelaxed(log.count("\n")) > NekoGui::dataStore->max_log_line) return;
                MW_show_log_ext_vt100(log);
            });
            connect(this, &QProcess::readyReadStandardError, this, [this]() {
                MW_show_log_ext_vt100(readAllStandardError().trimmed());
            });
            connect(this, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
                if (!killed) {
                    crashed = true;
                    MW_show_log_ext(tag, "errorOccurred:" + errorString());
                    MW_dialog_message("ExternalProcess", "Crashed");
                }
            });
            connect(this, &QProcess::stateChanged, this, [this](QProcess::ProcessState state) {
                if (state == QProcess::NotRunning) {
                    if (killed) { // 用户命令退出
                        MW_show_log_ext(tag, "External core stopped");
                    } else if (!crashed) { // 异常退出
                        crashed = true;
                        MW_show_log_ext(tag, "[Error] Program exited accidentally: " + errorString());
                        Kill();
                        MW_dialog_message("ExternalProcess", "Crashed");
                    }
                }
            });
            MW_show_log_ext(tag, "External core starting: " + env.join(" ") + " " + program + " " + arguments.join(" "));
        }

        QProcess::setEnvironment(env);
        QProcess::start(program, arguments);
    }

    void ExternalProcess::Kill() {
        if (killed) return;
        killed = true;

        if (!crashed) {
            QProcess::kill();
            QProcess::waitForFinished(500);
        }
    }

    //

    QElapsedTimer coreRestartTimer;

    CoreProcess::CoreProcess(const QString &core_path, const QStringList &args) : ExternalProcess() {
        ExternalProcess::managed = false;
        ExternalProcess::program = core_path;
        ExternalProcess::arguments = args;

        setup_stdout_handler();
        setup_stderr_handler();
        setup_state_handlers();
    }

    void CoreProcess::setup_stdout_handler() {
        // Debug: Log handler setup
        QString logPath = QDir::currentPath() + "/core_output.log";
        QFile setupLogFile(logPath);
        if (setupLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream setupStream(&setupLogFile);
            setupStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [SETUP] setup_stdout_handler() called\n";
            setupStream.flush();
            setupLogFile.close();
        }
        
        connect(this, &QProcess::readyReadStandardOutput, this, [this]() {
            auto log = readAllStandardOutput();
            QString logText = QString::fromUtf8(log);

            // Debug: Always log to core_output.log for troubleshooting
            static QFile debugLogFile;
            static bool fileInitialized = false;
            if (!fileInitialized) {
                QString logPath = QDir::currentPath() + "/core_output.log";
                debugLogFile.setFileName(logPath);
                if (debugLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                    fileInitialized = true;
                    // Write initial marker to confirm handler is working
                    QTextStream initStream(&debugLogFile);
                    initStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [INIT] stdout handler initialized, first data received\n";
                    initStream.flush();
                }
            }
            if (debugLogFile.isOpen()) {
                QTextStream debugStream(&debugLogFile);
                debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [STDOUT] " << log;
                debugStream.flush();
            }
            
            // Also log to UI immediately for debugging
            if (!log.isEmpty()) {
                MW_show_log("[Core Output] " + log.trimmed());
            }
            
            if (!NekoGui::dataStore->core_running) {
                if (logText.contains("grpc server listening", Qt::CaseInsensitive)) {
                    // The core really started
                    NekoGui::dataStore->core_running = true;
                    MW_show_log("[Core] gRPC server is listening");
                    if (start_profile_when_core_is_up >= 0) {
                        MW_dialog_message("ExternalProcess", "CoreStarted," + Int2String(start_profile_when_core_is_up));
                        start_profile_when_core_is_up = -1;
                    }
                } else if (logText.contains("failed to serve", Qt::CaseInsensitive)) {
                    // The core failed to start
                    MW_show_log("[Core] Failed to serve gRPC");
                    QProcess::kill();
                }
            }
            
            // Always show logs - don't filter by log_ignore for core output
            // Filter only if log counter exceeds limit
            if (logCounter.fetchAndAddRelaxed(log.count("\n")) > NekoGui::dataStore->max_log_line) return;
            
            // Show all core output (INFO, ERROR, WARN, DEBUG)
            // Force display even if empty (to ensure we see all output)
            if (!log.isEmpty()) {
                MW_show_log(log);
            }
        });
    }

    void CoreProcess::setup_stderr_handler() {
        // Note: With MergedChannels, stderr is captured via stdout handler
        // But we keep this for compatibility and to enable stderr display
        connect(this, &QProcess::readyReadStandardError, this, [this]() {
            auto log = readAllStandardError().trimmed();

            // Debug: Always log to core_output.log for troubleshooting
            static QFile debugLogFile;
            static bool fileInitialized = false;
            if (!fileInitialized) {
                QString logPath = QDir::currentPath() + "/core_output.log";
                debugLogFile.setFileName(logPath);
                if (debugLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                    fileInitialized = true;
                }
            }
            if (debugLogFile.isOpen()) {
                QTextStream debugStream(&debugLogFile);
                debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [STDERR] " << log << "\n";
                debugStream.flush();
            }
            
            if (show_stderr) {
                MW_show_log(log);
                return;
            }
            if (log.contains("token is set")) {
                show_stderr = true;
            }
            // Always show ERROR and INFO level logs
            if (log.contains("ERROR[") || log.contains("INFO[") || log.contains("WARN[") || log.contains("DEBUG[")) {
                MW_show_log(log);
            }
        });
        connect(this, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::ProcessError::FailedToStart) {
                failed_to_start = true;
                MW_show_log("start core error occurred: " + errorString() + "\n");
            }
        });
    }

    void CoreProcess::setup_state_handlers() {
        connect(this, &QProcess::stateChanged, this, [this](QProcess::ProcessState state) {
            if (state == QProcess::NotRunning) {
                NekoGui::dataStore->core_running = false;
            }

            if (!NekoGui::dataStore->prepare_exit && state == QProcess::NotRunning) {
                if (failed_to_start) return; // no retry
                if (restarting) return;

                MW_dialog_message("ExternalProcess", "CoreCrashed");

                // Retry rate limit
                if (coreRestartTimer.isValid()) {
                    if (coreRestartTimer.restart() < 10 * 1000) {
                        coreRestartTimer = QElapsedTimer();
                        MW_show_log("[Error] " + QObject::tr("Core exits too frequently, stop automatic restart this profile."));
                        return;
                    }
                } else {
                    coreRestartTimer.start();
                }

                // Restart
                start_profile_when_core_is_up = NekoGui::dataStore->started_id;
                MW_show_log("[Error] " + QObject::tr("Core exited, restarting."));
                setTimeout([=] { Restart(); }, this, 1000);
            }
        });
    }

    void CoreProcess::Start() {
        show_stderr = false;
        // cwd: same as GUI, at ./config
        // Ensure we capture both stdout and stderr
        setProcessChannelMode(QProcess::MergedChannels);
        
        // Debug: Log that we're starting
        QString logPath = QDir::currentPath() + "/core_output.log";
        QFile debugLogFile(logPath);
        if (debugLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream debugStream(&debugLogFile);
            debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [START] CoreProcess::Start() called\n";
            debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [START] Program: " << program << "\n";
            debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [START] Arguments: " << arguments.join(" ") << "\n";
            debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [START] Channel mode: MergedChannels\n";
            debugStream.flush();
            debugLogFile.close();
        }
        
        ExternalProcess::Start();
        write((NekoGui::dataStore->core_token + "\n").toUtf8());
        
        // Debug: Log after start
        if (debugLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream debugStream(&debugLogFile);
            debugStream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [START] Process started, state: " << state() << "\n";
            debugStream.flush();
            debugLogFile.close();
        }
    }

    void CoreProcess::Restart() {
        restarting = true;
        QProcess::kill();
        QProcess::waitForFinished(500);
        ExternalProcess::started = false;
        Start();
        restarting = false;
    }

} // namespace NekoGui_sys
