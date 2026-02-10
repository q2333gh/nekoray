#include "./ui_mainwindow.h"
#include "mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <QProcess>

// Helper function to download core executable
bool DownloadCoreExecutable(const QString &destDir) {
    WriteCrashLog(QString("Attempting to download core executable to: %1").arg(destDir));
    
#ifdef Q_OS_WIN
    // Find Python script
    QString repoRoot = QDir(QApplication::applicationDirPath()).absolutePath();
    // Try to find repo root by looking for libs/download_core.py
    QString scriptPath = repoRoot + "/libs/download_core.py";
    QFileInfo scriptInfo(scriptPath);
    
    // If not found in application dir, try going up directories
    if (!scriptInfo.exists()) {
        QDir dir(repoRoot);
        for (int i = 0; i < 5 && !scriptInfo.exists(); i++) {
            dir.cdUp();
            scriptPath = dir.absolutePath() + "/libs/download_core.py";
            scriptInfo.setFile(scriptPath);
        }
    }
    
    if (!scriptInfo.exists()) {
        WriteCrashLog(QString("CRITICAL: download_core.py not found. Tried: %1").arg(scriptPath));
        return false;
    }
    
    WriteCrashLog(QString("Found download script at: %1").arg(scriptPath));
    
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
    
    // Execute Python script
    QProcess process;
    process.setProgram(pythonExe);
    QStringList args;
    args << scriptPath;
    args << "--dest-dir" << destDir;
    args << "--version" << "latest";
    
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    WriteCrashLog(QString("Starting Python download process with: %1").arg(pythonExe));
    process.start();
    
    if (!process.waitForStarted(5000)) {
        WriteCrashLog("CRITICAL: Failed to start Python process");
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
