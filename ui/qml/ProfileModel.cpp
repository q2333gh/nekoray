#include "ProfileModel.hpp"
#include "db/Database.hpp"
#include "main/NekoGui_Utils.hpp"
#include "fmt/AbstractBean.hpp"
#include "fmt/SocksHttpBean.hpp"
#include "fmt/ShadowSocksBean.hpp"
#include "fmt/VMessBean.hpp"
#include "fmt/TrojanVLESSBean.hpp"
#include "fmt/NaiveBean.hpp"
#include "fmt/QUICBean.hpp"
#include <QColor>

ProfileModel::ProfileModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int ProfileModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return m_profiles.size();
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_profiles.size()) {
        return QVariant();
    }

    auto profile = m_profiles[index.row()];
    if (!profile) {
        return QVariant();
    }

    switch (role) {
        case ProxyIdRole:
            return profile->id;
        case NameRole:
            return getProfileName(profile);
        case TypeRole:
            return profile->type;
        case AddressRole:
            return getProfileAddress(profile);
        case PortRole:
            return getProfilePort(profile);
        case LatencyRole:
            return profile->latency;
        case LatencyDisplayRole:
            return profile->DisplayLatency();
        case LatencyColorRole:
            return profile->DisplayLatencyColor();
        case TrafficRole: {
            if (profile->traffic_data) {
                return profile->traffic_data->DisplayTraffic();
            }
            return QString();
        }
        case IsRunningRole:
            return profile->id == NekoGui::dataStore->started_id;
        case GroupIdRole:
            return profile->gid;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ProfileModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ProxyIdRole] = "proxyId";
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[AddressRole] = "address";
    roles[PortRole] = "port";
    roles[LatencyRole] = "latency";
    roles[LatencyDisplayRole] = "latencyDisplay";
    roles[LatencyColorRole] = "latencyColor";
    roles[TrafficRole] = "traffic";
    roles[IsRunningRole] = "isRunning";
    roles[GroupIdRole] = "groupId";
    return roles;
}

void ProfileModel::refresh(int groupId) {
    beginResetModel();
    m_profiles.clear();
    m_currentGroupId = groupId;

    if (groupId < 0) {
        auto group = NekoGui::profileManager->CurrentGroup();
        if (group) {
            m_profiles = group->ProfilesWithOrder();
        }
    } else {
        auto group = NekoGui::profileManager->GetGroup(groupId);
        if (group) {
            m_profiles = group->ProfilesWithOrder();
        }
    }

    endResetModel();
}

void ProfileModel::updateProfile(int id) {
    auto profile = NekoGui::profileManager->GetProfile(id);
    if (!profile) return;

    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i]->id == id) {
            m_profiles[i] = profile;
            QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx);
            break;
        }
    }
}

QVariantMap ProfileModel::getProfile(int index) const {
    QVariantMap result;
    if (index < 0 || index >= m_profiles.size()) {
        return result;
    }

    auto profile = m_profiles[index];
    result["id"] = profile->id;
    result["name"] = getProfileName(profile);
    result["type"] = profile->type;
    result["address"] = getProfileAddress(profile);
    result["port"] = getProfilePort(profile);
    result["latency"] = profile->latency;
    result["latencyDisplay"] = profile->DisplayLatency();
    result["groupId"] = profile->gid;
    return result;
}

QString ProfileModel::getProfileName(const std::shared_ptr<NekoGui::ProxyEntity> &profile) const {
    if (!profile || !profile->bean) return QString();
    return profile->bean->DisplayName();
}

QString ProfileModel::getProfileAddress(const std::shared_ptr<NekoGui::ProxyEntity> &profile) const {
    if (!profile || !profile->bean) return QString();
    return profile->bean->DisplayAddress();
}

int ProfileModel::getProfilePort(const std::shared_ptr<NekoGui::ProxyEntity> &profile) const {
    if (!profile || !profile->bean) return 0;
    return profile->bean->serverPort;
}
