#include "MainWindowController.hpp"
#include "ProfileModel.hpp"
#include "GroupModel.hpp"
#include "MainWindowAdapter.hpp"
#include "db/Database.hpp"
#include "main/NekoGui.hpp"
#include "main/NekoGui_Utils.hpp"

MainWindowController::MainWindowController(QObject *parent)
    : QObject(parent)
    , m_profileModel(new ProfileModel(this))
    , m_groupModel(new GroupModel(this)) {
    
    // Initialize models
    m_groupModel->refresh();
    auto currentGroup = NekoGui::profileManager->CurrentGroup();
    if (currentGroup) {
        m_currentGroupId = currentGroup->id;
        m_profileModel->refresh(m_currentGroupId);
    }
    
    updateRunningState();
}

MainWindowController::~MainWindowController() {
}

void MainWindowController::start(int profileId) {
    if (profileId < 0) {
        // Get first selected profile or use current
        auto currentGroup = NekoGui::profileManager->CurrentGroup();
        if (currentGroup) {
            auto profiles = currentGroup->ProfilesWithOrder();
            if (!profiles.isEmpty()) {
                profileId = profiles.first()->id;
            }
        }
    }
    
    if (profileId >= 0) {
        neko_start_impl(profileId);
        updateRunningState();
    }
}

void MainWindowController::stop() {
    neko_stop_impl(false);
    updateRunningState();
}

void MainWindowController::refreshProxyList(int groupId) {
    if (groupId < 0) {
        groupId = m_currentGroupId;
    }
    m_profileModel->refresh(groupId);
    emit currentGroupIdChanged();
}

void MainWindowController::showGroup(int groupId) {
    show_group_impl(groupId);
    m_currentGroupId = groupId;
    m_profileModel->refresh(groupId);
    emit currentGroupIdChanged();
}

void MainWindowController::refreshGroups() {
    refresh_groups_impl();
    m_groupModel->refresh();
    updateCurrentGroup();
}

void MainWindowController::refreshStatus(const QString &trafficUpdate) {
    refresh_status_impl(trafficUpdate);
    if (!trafficUpdate.isEmpty()) {
        m_trafficUpdate = trafficUpdate;
        emit trafficUpdateChanged();
    }
}

void MainWindowController::setSystemProxy(bool enable) {
    neko_set_spmode_system_proxy_impl(enable, true);
}

void MainWindowController::setVPN(bool enable) {
    neko_set_spmode_vpn_impl(enable, true);
}

void MainWindowController::showLog(const QString &log) {
    show_log_impl(log);
    m_logText += log + "\n";
    emit logTextChanged();
}

void MainWindowController::refreshConnectionList(const QJsonArray &arr) {
    refresh_connection_list_impl(arr);
}

void MainWindowController::speedtestCurrentGroup() {
    speedtest_current_group_impl(1, true);
}

void MainWindowController::speedtestCurrent() {
    speedtest_current_impl();
}

void MainWindowController::addProfileFromClipboard() {
    // TODO: Implement - adapt from MainWindow::on_menu_add_from_clipboard_triggered
    emit showMessage("Info", "Add from clipboard - to be implemented");
}

void MainWindowController::addProfileFromInput() {
    // Signal to QML to open new profile dialog
    auto currentGroup = NekoGui::profileManager->CurrentGroup();
    int groupId = currentGroup ? currentGroup->id : -1;
    emit newProfileRequested("socks", groupId);
}

void MainWindowController::cloneProfile(int profileId) {
    auto profile = NekoGui::profileManager->GetProfile(profileId);
    if (!profile) return;
    
    // TODO: Implement clone logic
    emit showMessage("Info", "Clone profile - to be implemented");
}

void MainWindowController::deleteProfile(int profileId) {
    NekoGui::profileManager->DeleteProfile(profileId);
    refreshProxyList();
}

void MainWindowController::moveProfile(int profileId, int groupId) {
    auto profile = NekoGui::profileManager->GetProfile(profileId);
    if (profile) {
        NekoGui::profileManager->MoveProfile(profile, groupId);
        refreshProxyList(groupId);
    }
}

void MainWindowController::resetTraffic(int profileId) {
    auto profile = NekoGui::profileManager->GetProfile(profileId);
    if (profile && profile->traffic_data) {
        profile->traffic_data->uplink = 0;
        profile->traffic_data->downlink = 0;
        profile->traffic_data->uplink_rate = 0;
        profile->traffic_data->downlink_rate = 0;
        profile->Save();
        m_profileModel->updateProfile(profileId);
    }
}

void MainWindowController::showBasicSettings() {
    // This will be handled by QML - open BasicSettingsDialog
    emit showMessage("Info", "Basic settings dialog");
}

void MainWindowController::showRoutingSettings() {
    // TODO: Open ManageRoutesDialog
    emit showMessage("Info", "Routing settings - to be implemented");
}

void MainWindowController::showVPNSettings() {
    // TODO: Open VPNSettingsDialog
    emit showMessage("Info", "VPN settings - to be implemented");
}

void MainWindowController::showHotkeySettings() {
    // TODO: Open HotkeyDialog
    emit showMessage("Info", "Hotkey settings - to be implemented");
}

void MainWindowController::showManageGroups() {
    // TODO: Open ManageGroupsDialog
    emit showMessage("Info", "Manage groups - to be implemented");
}

void MainWindowController::showEditProfile(int profileId) {
    // Signal to QML to open edit dialog
    emit editProfileRequested(profileId);
}

void MainWindowController::showEditGroup(int groupId) {
    // TODO: Open EditGroupDialog
    emit showMessage("Info", "Edit group - to be implemented");
}

void MainWindowController::commitDataRequest() {
    // Save window size, splitter state, etc.
    NekoGui::dataStore->Save();
    NekoGui::profileManager->SaveManager();
}

void MainWindowController::updateRunningState() {
    bool wasRunning = m_isRunning;
    int wasProfileId = m_currentProfileId;
    
    m_isRunning = NekoGui::dataStore->started_id >= 0;
    m_currentProfileId = NekoGui::dataStore->started_id;
    
    if (wasRunning != m_isRunning) {
        emit isRunningChanged();
    }
    if (wasProfileId != m_currentProfileId) {
        emit currentProfileIdChanged();
        if (m_currentProfileId >= 0) {
            m_profileModel->updateProfile(m_currentProfileId);
        }
    }
}

void MainWindowController::updateCurrentGroup() {
    auto currentGroup = NekoGui::profileManager->CurrentGroup();
    int newGroupId = currentGroup ? currentGroup->id : -1;
    
    if (m_currentGroupId != newGroupId) {
        m_currentGroupId = newGroupId;
        emit currentGroupIdChanged();
        m_profileModel->refresh(m_currentGroupId);
    }
}

void MainWindowController::onProfileStarted(int id) {
    m_currentProfileId = id;
    m_isRunning = true;
    emit isRunningChanged();
    emit currentProfileIdChanged();
    m_profileModel->updateProfile(id);
}

void MainWindowController::onProfileStopped() {
    m_isRunning = false;
    emit isRunningChanged();
}

void MainWindowController::onGroupChanged(int groupId) {
    m_currentGroupId = groupId;
    emit currentGroupIdChanged();
    m_profileModel->refresh(groupId);
}
