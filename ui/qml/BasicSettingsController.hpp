#pragma once

#include <QObject>

class BasicSettingsController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString inboundAddress READ inboundAddress WRITE setInboundAddress NOTIFY inboundAddressChanged)
    Q_PROPERTY(int inboundSocksPort READ inboundSocksPort WRITE setInboundSocksPort NOTIFY inboundSocksPortChanged)
    Q_PROPERTY(QString logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
    Q_PROPERTY(QString testLatencyUrl READ testLatencyUrl WRITE setTestLatencyUrl NOTIFY testLatencyUrlChanged)
    Q_PROPERTY(QString testDownloadUrl READ testDownloadUrl WRITE setTestDownloadUrl NOTIFY testDownloadUrlChanged)
    Q_PROPERTY(int testConcurrent READ testConcurrent WRITE setTestConcurrent NOTIFY testConcurrentChanged)
    Q_PROPERTY(int testDownloadTimeout READ testDownloadTimeout WRITE setTestDownloadTimeout NOTIFY testDownloadTimeoutChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool startMinimal READ startMinimal WRITE setStartMinimal NOTIFY startMinimalChanged)
    Q_PROPERTY(bool connectionStatistics READ connectionStatistics WRITE setConnectionStatistics NOTIFY connectionStatisticsChanged)

public:
    explicit BasicSettingsController(QObject *parent = nullptr);

    QString inboundAddress() const { return m_inboundAddress; }
    void setInboundAddress(const QString &value);

    int inboundSocksPort() const { return m_inboundSocksPort; }
    void setInboundSocksPort(int value);

    QString logLevel() const { return m_logLevel; }
    void setLogLevel(const QString &value);

    QString testLatencyUrl() const { return m_testLatencyUrl; }
    void setTestLatencyUrl(const QString &value);

    QString testDownloadUrl() const { return m_testDownloadUrl; }
    void setTestDownloadUrl(const QString &value);

    int testConcurrent() const { return m_testConcurrent; }
    void setTestConcurrent(int value);

    int testDownloadTimeout() const { return m_testDownloadTimeout; }
    void setTestDownloadTimeout(int value);

    QString theme() const { return m_theme; }
    void setTheme(const QString &value);

    bool startMinimal() const { return m_startMinimal; }
    void setStartMinimal(bool value);

    bool connectionStatistics() const { return m_connectionStatistics; }
    void setConnectionStatistics(bool value);

    Q_INVOKABLE void loadSettings();
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void editCustomInbound();

signals:
    void inboundAddressChanged();
    void inboundSocksPortChanged();
    void logLevelChanged();
    void testLatencyUrlChanged();
    void testDownloadUrlChanged();
    void testConcurrentChanged();
    void testDownloadTimeoutChanged();
    void themeChanged();
    void startMinimalChanged();
    void connectionStatisticsChanged();

private:
    QString m_inboundAddress;
    int m_inboundSocksPort = 10808;
    QString m_logLevel = "info";
    QString m_testLatencyUrl;
    QString m_testDownloadUrl;
    int m_testConcurrent = 1;
    int m_testDownloadTimeout = 5;
    QString m_theme = "0";
    bool m_startMinimal = false;
    bool m_connectionStatistics = false;
};
