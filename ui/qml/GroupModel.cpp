#include "GroupModel.hpp"
#include "db/Database.hpp"

GroupModel::GroupModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int GroupModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_groups.size();
}

QVariant GroupModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_groups.size()) {
        return QVariant();
    }

    auto group = m_groups[index.row()];
    if (!group) {
        return QVariant();
    }

    switch (role) {
        case GroupIdRole:
            return group->id;
        case NameRole:
            return group->name;
        case UrlRole:
            return group->url;
        case InfoRole:
            return group->info;
        case ProfileCountRole: {
            auto profiles = group->Profiles();
            return profiles.size();
        }
        case IsCurrentRole: {
            auto currentGroup = NekoGui::profileManager->CurrentGroup();
            return currentGroup && currentGroup->id == group->id;
        }
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> GroupModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[GroupIdRole] = "groupId";
    roles[NameRole] = "name";
    roles[UrlRole] = "url";
    roles[InfoRole] = "info";
    roles[ProfileCountRole] = "profileCount";
    roles[IsCurrentRole] = "isCurrent";
    return roles;
}

void GroupModel::refresh() {
    beginResetModel();
    m_groups.clear();

    // Get groups in tab order
    for (int gid : NekoGui::profileManager->groupsTabOrder) {
        auto group = NekoGui::profileManager->GetGroup(gid);
        if (group) {
            m_groups.append(group);
        }
    }

    endResetModel();
}

QVariantMap GroupModel::getGroup(int index) const {
    QVariantMap result;
    if (index < 0 || index >= m_groups.size()) {
        return result;
    }

    auto group = m_groups[index];
    result["id"] = group->id;
    result["name"] = group->name;
    result["url"] = group->url;
    result["info"] = group->info;
    auto profiles = group->Profiles();
    result["profileCount"] = profiles.size();
    
    auto currentGroup = NekoGui::profileManager->CurrentGroup();
    result["isCurrent"] = currentGroup && currentGroup->id == group->id;
    
    return result;
}
