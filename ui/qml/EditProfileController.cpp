#include "EditProfileController.hpp"
#include "db/Database.hpp"
#include "fmt/includes.h"

EditProfileController::EditProfileController(QObject *parent)
    : QObject(parent) {
}

void EditProfileController::setProfileType(const QString &value) {
    if (m_profileType != value) {
        m_profileType = value;
        emit profileTypeChanged();
    }
}

void EditProfileController::setProfileName(const QString &value) {
    if (m_profileName != value) {
        m_profileName = value;
        emit profileNameChanged();
    }
}

void EditProfileController::setServerAddress(const QString &value) {
    if (m_serverAddress != value) {
        m_serverAddress = value;
        emit serverAddressChanged();
    }
}

void EditProfileController::setServerPort(int value) {
    if (m_serverPort != value) {
        m_serverPort = value;
        emit serverPortChanged();
    }
}

void EditProfileController::setProfileId(int value) {
    if (m_profileId != value) {
        m_profileId = value;
        emit profileIdChanged();
    }
}

void EditProfileController::setGroupId(int value) {
    if (m_groupId != value) {
        m_groupId = value;
        emit groupIdChanged();
    }
}

void EditProfileController::loadProfile(int id) {
    auto profile = NekoGui::profileManager->GetProfile(id);
    if (!profile) return;

    m_profileId = id;
    m_profileType = profile->type;
    m_groupId = profile->gid;

    if (profile->bean) {
        m_profileName = profile->bean->name;
        m_serverAddress = profile->bean->serverAddress;
        m_serverPort = profile->bean->serverPort;
    }

    emit profileIdChanged();
    emit profileTypeChanged();
    emit profileNameChanged();
    emit serverAddressChanged();
    emit serverPortChanged();
    emit groupIdChanged();
}

void EditProfileController::createNew(const QString &type, int gid) {
    m_profileType = type;
    m_groupId = gid;
    m_profileId = -1;
    m_profileName = "";
    m_serverAddress = "127.0.0.1";
    m_serverPort = 1080;

    emit profileTypeChanged();
    emit groupIdChanged();
    emit profileIdChanged();
    emit profileNameChanged();
    emit serverAddressChanged();
    emit serverPortChanged();
}

bool EditProfileController::save() {
    std::shared_ptr<NekoGui::ProxyEntity> profile;

    if (m_profileId >= 0) {
        profile = NekoGui::profileManager->GetProfile(m_profileId);
        if (!profile) return false;
    } else {
        profile = NekoGui::ProfileManager::NewProxyEntity(m_profileType);
        if (!profile) return false;
        profile->gid = m_groupId;
    }

    if (profile->bean) {
        profile->bean->name = m_profileName;
        profile->bean->serverAddress = m_serverAddress;
        profile->bean->serverPort = m_serverPort;
    }

    if (m_profileId < 0) {
        if (!NekoGui::profileManager->AddProfile(profile, m_groupId)) {
            return false;
        }
    } else {
        profile->Save();
    }

    emit saved();
    return true;
}
