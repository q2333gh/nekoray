#include "BasicSettingsController.hpp"
#include "main/NekoGui.hpp"
#include "ui/ThemeManager.hpp"

BasicSettingsController::BasicSettingsController(QObject *parent)
    : QObject(parent) {
    loadSettings();
}

void BasicSettingsController::setInboundAddress(const QString &value) {
    if (m_inboundAddress != value) {
        m_inboundAddress = value;
        emit inboundAddressChanged();
    }
}

void BasicSettingsController::setInboundSocksPort(int value) {
    if (m_inboundSocksPort != value) {
        m_inboundSocksPort = value;
        emit inboundSocksPortChanged();
    }
}

void BasicSettingsController::setLogLevel(const QString &value) {
    if (m_logLevel != value) {
        m_logLevel = value;
        emit logLevelChanged();
    }
}

void BasicSettingsController::setTestLatencyUrl(const QString &value) {
    if (m_testLatencyUrl != value) {
        m_testLatencyUrl = value;
        emit testLatencyUrlChanged();
    }
}

void BasicSettingsController::setTestDownloadUrl(const QString &value) {
    if (m_testDownloadUrl != value) {
        m_testDownloadUrl = value;
        emit testDownloadUrlChanged();
    }
}

void BasicSettingsController::setTestConcurrent(int value) {
    if (m_testConcurrent != value) {
        m_testConcurrent = value;
        emit testConcurrentChanged();
    }
}

void BasicSettingsController::setTestDownloadTimeout(int value) {
    if (m_testDownloadTimeout != value) {
        m_testDownloadTimeout = value;
        emit testDownloadTimeoutChanged();
    }
}

void BasicSettingsController::setTheme(const QString &value) {
    if (m_theme != value) {
        m_theme = value;
        emit themeChanged();
    }
}

void BasicSettingsController::setStartMinimal(bool value) {
    if (m_startMinimal != value) {
        m_startMinimal = value;
        emit startMinimalChanged();
    }
}

void BasicSettingsController::setConnectionStatistics(bool value) {
    if (m_connectionStatistics != value) {
        m_connectionStatistics = value;
        emit connectionStatisticsChanged();
    }
}

void BasicSettingsController::loadSettings() {
    m_inboundAddress = NekoGui::dataStore->inbound_address;
    m_inboundSocksPort = NekoGui::dataStore->inbound_socks_port;
    m_logLevel = NekoGui::dataStore->log_level;
    m_testLatencyUrl = NekoGui::dataStore->test_latency_url;
    m_testDownloadUrl = NekoGui::dataStore->test_download_url;
    m_testConcurrent = NekoGui::dataStore->test_concurrent;
    m_testDownloadTimeout = NekoGui::dataStore->test_download_timeout;
    m_theme = NekoGui::dataStore->theme;
    m_startMinimal = NekoGui::dataStore->start_minimal;
    m_connectionStatistics = NekoGui::dataStore->connection_statistics;
}

void BasicSettingsController::saveSettings() {
    NekoGui::dataStore->inbound_address = m_inboundAddress;
    NekoGui::dataStore->inbound_socks_port = m_inboundSocksPort;
    NekoGui::dataStore->log_level = m_logLevel;
    NekoGui::dataStore->test_latency_url = m_testLatencyUrl;
    NekoGui::dataStore->test_download_url = m_testDownloadUrl;
    NekoGui::dataStore->test_concurrent = m_testConcurrent;
    NekoGui::dataStore->test_download_timeout = m_testDownloadTimeout;
    NekoGui::dataStore->theme = m_theme;
    NekoGui::dataStore->start_minimal = m_startMinimal;
    NekoGui::dataStore->connection_statistics = m_connectionStatistics;
    
    themeManager->ApplyTheme(m_theme);
    NekoGui::dataStore->Save();
}

void BasicSettingsController::editCustomInbound() {
    // TODO: Open JSON editor dialog
}
