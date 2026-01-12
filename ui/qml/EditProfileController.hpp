#pragma once

#include <QObject>
#include <QString>

class EditProfileController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString profileType READ profileType WRITE setProfileType NOTIFY profileTypeChanged)
    Q_PROPERTY(QString profileName READ profileName WRITE setProfileName NOTIFY profileNameChanged)
    Q_PROPERTY(QString serverAddress READ serverAddress WRITE setServerAddress NOTIFY serverAddressChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY serverPortChanged)
    Q_PROPERTY(int profileId READ profileId WRITE setProfileId NOTIFY profileIdChanged)
    Q_PROPERTY(int groupId READ groupId WRITE setGroupId NOTIFY groupIdChanged)

public:
    explicit EditProfileController(QObject *parent = nullptr);

    QString profileType() const { return m_profileType; }
    void setProfileType(const QString &value);

    QString profileName() const { return m_profileName; }
    void setProfileName(const QString &value);

    QString serverAddress() const { return m_serverAddress; }
    void setServerAddress(const QString &value);

    int serverPort() const { return m_serverPort; }
    void setServerPort(int value);

    int profileId() const { return m_profileId; }
    void setProfileId(int value);

    int groupId() const { return m_groupId; }
    void setGroupId(int value);

    Q_INVOKABLE void loadProfile(int id);
    Q_INVOKABLE void createNew(const QString &type, int gid);
    Q_INVOKABLE bool save();

signals:
    void profileTypeChanged();
    void profileNameChanged();
    void serverAddressChanged();
    void serverPortChanged();
    void profileIdChanged();
    void groupIdChanged();
    void saved();

private:
    QString m_profileType;
    QString m_profileName;
    QString m_serverAddress;
    int m_serverPort = 1080;
    int m_profileId = -1;
    int m_groupId = -1;
};
