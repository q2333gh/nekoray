#pragma once

#include <QAbstractListModel>
#include "db/Group.hpp"

class GroupModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum GroupRoles {
        GroupIdRole = Qt::UserRole + 1,
        NameRole,
        UrlRole,
        InfoRole,
        ProfileCountRole,
        IsCurrentRole
    };

    explicit GroupModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap getGroup(int index) const;

private:
    QList<std::shared_ptr<NekoGui::Group>> m_groups;
};
