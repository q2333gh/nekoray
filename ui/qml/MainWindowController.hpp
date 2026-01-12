#pragma once

#include <QObject>
#include <QString>
#include <QJsonArray>

#include "ProfileModel.hpp"
#include "GroupModel.hpp"

class MainWindowController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int currentProfileId READ currentProfileId NOTIFY currentProfileIdChanged)
    Q_PROPERTY(int currentGroupId READ currentGroupId NOTIFY currentGroupIdChanged)
    Q_PROPERTY(QString trafficUpdate READ trafficUpdate NOTIFY trafficUpdateChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(ProfileModel* profileModel READ profileModel CONSTANT)
    Q_PROPERTY(GroupModel* groupModel READ groupModel CONSTANT)

public:
    explicit MainWindowController(QObject *parent = nullptr);
    ~MainWindowController();

    bool isRunning() const { return m_isRunning; }
    int currentProfileId() const { return m_currentProfileId; }
    int currentGroupId() const { return m_currentGroupId; }
    QString trafficUpdate() const { return m_trafficUpdate; }
    QString logText() const { return m_logText; }
    ProfileModel* profileModel() const { return m_profileModel; }
    GroupModel* groupModel() const { return m_groupModel; }

public slots:
    Q_INVOKABLE void start(int profileId = -1);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void refreshProxyList(int groupId = -1);
    Q_INVOKABLE void showGroup(int groupId);
    Q_INVOKABLE void refreshGroups();
    Q_INVOKABLE void refreshStatus(const QString &trafficUpdate = "");
    Q_INVOKABLE void setSystemProxy(bool enable);
    Q_INVOKABLE void setVPN(bool enable);
    Q_INVOKABLE void showLog(const QString &log);
    Q_INVOKABLE void refreshConnectionList(const QJsonArray &arr);
    Q_INVOKABLE void speedtestCurrentGroup();
    Q_INVOKABLE void speedtestCurrent();
    Q_INVOKABLE void addProfileFromClipboard();
    Q_INVOKABLE void addProfileFromInput();
    Q_INVOKABLE void cloneProfile(int profileId);
    Q_INVOKABLE void deleteProfile(int profileId);
    Q_INVOKABLE void moveProfile(int profileId, int groupId);
    Q_INVOKABLE void resetTraffic(int profileId);
    Q_INVOKABLE void showBasicSettings();
    Q_INVOKABLE void showRoutingSettings();
    Q_INVOKABLE void showVPNSettings();
    Q_INVOKABLE void showHotkeySettings();
    Q_INVOKABLE void showManageGroups();
    Q_INVOKABLE void showEditProfile(int profileId);
    Q_INVOKABLE void showEditGroup(int groupId);
    Q_INVOKABLE void commitDataRequest();

signals:
    void isRunningChanged();
    void currentProfileIdChanged();
    void currentGroupIdChanged();
    void trafficUpdateChanged();
    void logTextChanged();
    void profileSelected(int id);
    void showMessage(const QString &title, const QString &message);
    void editProfileRequested(int profileId);
    void newProfileRequested(const QString &type, int groupId);

private slots:
    void onProfileStarted(int id);
    void onProfileStopped();
    void onGroupChanged(int groupId);

private:
    bool m_isRunning = false;
    int m_currentProfileId = -1;
    int m_currentGroupId = -1;
    QString m_trafficUpdate;
    QString m_logText;
    ProfileModel *m_profileModel;
    GroupModel *m_groupModel;

    void updateRunningState();
    void updateCurrentGroup();
};
