#pragma once

#include <QAbstractListModel>
#include <QColor>
#include "db/ProxyEntity.hpp"

class ProfileModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum ProfileRoles {
        ProxyIdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        AddressRole,
        PortRole,
        LatencyRole,
        LatencyDisplayRole,
        LatencyColorRole,
        TrafficRole,
        IsRunningRole,
        GroupIdRole
    };

    explicit ProfileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh(int groupId = -1);
    Q_INVOKABLE void updateProfile(int id);
    Q_INVOKABLE QVariantMap getProfile(int index) const;

private:
    QList<std::shared_ptr<NekoGui::ProxyEntity>> m_profiles;
    int m_currentGroupId = -1;

    QString getProfileName(const std::shared_ptr<NekoGui::ProxyEntity> &profile) const;
    QString getProfileAddress(const std::shared_ptr<NekoGui::ProxyEntity> &profile) const;
    int getProfilePort(const std::shared_ptr<NekoGui::ProxyEntity> &profile) const;
};
