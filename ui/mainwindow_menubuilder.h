#pragma once

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QActionGroup>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow;

/**
 * MenuBuilder - Dynamically creates all menus and actions for MainWindow
 * This allows us to keep the UI file minimal (< 400 lines) by moving menu definitions to code
 */
class MenuBuilder {
public:
    explicit MenuBuilder(MainWindow* parent);
    
    // Build all menus and actions, attach to menubar
    void buildMenus(QMenuBar* menubar);
    
    // Get menu references (for connections in MainWindow)
    QMenu* menuProgram() const { return m_menuProgram; }
    QMenu* menuPreferences() const { return m_menuPreferences; }
    QMenu* menuServer() const { return m_menuServer; }
    QMenu* menuSpmode() const { return m_menuSpmode; }
    QMenu* menuProgramPreference() const { return m_menuProgramPreference; }
    QMenu* menuActiveServer() const { return m_menuActiveServer; }
    QMenu* menuActiveRouting() const { return m_menuActiveRouting; }
    QMenu* menuShareItem() const { return m_menuShareItem; }
    QMenu* menuCurrentGroup() const { return m_menuCurrentGroup; }
    QMenu* menuCurrentSelect() const { return m_menuCurrentSelect; }
    
    // Get action references (for connections) - Program menu
    QAction* actionShowWindow() const { return m_actionShowWindow; }
    QAction* actionExit() const { return m_actionExit; }
    QAction* actionRestartProxy() const { return m_actionRestartProxy; }
    QAction* actionRestartProgram() const { return m_actionRestartProgram; }
    QAction* actionStartWithSystem() const { return m_actionStartWithSystem; }
    QAction* actionRememberLastProxy() const { return m_actionRememberLastProxy; }
    QAction* actionAllowLAN() const { return m_actionAllowLAN; }
    QAction* actionSpmodeSystemProxy() const { return m_actionSpmodeSystemProxy; }
    QAction* actionSpmodeVPN() const { return m_actionSpmodeVPN; }
    QAction* actionSpmodeDisabled() const { return m_actionSpmodeDisabled; }
    QAction* actionAddFromClipboard2() const { return m_actionAddFromClipboard2; }
    QAction* actionScanQR() const { return m_actionScanQR; }
    
    // Server menu actions
    QAction* actionStart() const { return m_actionStart; }
    QAction* actionStop() const { return m_actionStop; }
    QAction* actionAddFromInput() const { return m_actionAddFromInput; }
    QAction* actionAddFromClipboard() const { return m_actionAddFromClipboard; }
    QAction* actionSelectAll() const { return m_actionSelectAll; }
    QAction* actionMove() const { return m_actionMove; }
    QAction* actionClone() const { return m_actionClone; }
    QAction* actionResetTraffic() const { return m_actionResetTraffic; }
    QAction* actionDelete() const { return m_actionDelete; }
    QAction* actionQR() const { return m_actionQR; }
    QAction* actionExportConfig() const { return m_actionExportConfig; }
    QAction* actionCopyLinks() const { return m_actionCopyLinks; }
    QAction* actionCopyLinksNKR() const { return m_actionCopyLinksNKR; }
    QAction* actionTCPPing() const { return m_actionTCPPing; }
    QAction* actionURLTest() const { return m_actionURLTest; }
    QAction* actionFullTest() const { return m_actionFullTest; }
    QAction* actionStopTesting() const { return m_actionStopTesting; }
    QAction* actionClearTestResult() const { return m_actionClearTestResult; }
    QAction* actionResolveDomain() const { return m_actionResolveDomain; }
    QAction* actionRemoveUnavailable() const { return m_actionRemoveUnavailable; }
    QAction* actionDeleteRepeat() const { return m_actionDeleteRepeat; }
    QAction* actionUpdateSubscription() const { return m_actionUpdateSubscription; }
    QAction* actionProfileDebugInfo() const { return m_actionProfileDebugInfo; }
    
    // Preferences menu actions
    QAction* actionManageGroups() const { return m_actionManageGroups; }
    QAction* actionBasicSettings() const { return m_actionBasicSettings; }
    QAction* actionRoutingSettings() const { return m_actionRoutingSettings; }
    QAction* actionVPNSettings() const { return m_actionVPNSettings; }
    QAction* actionHotkeySettings() const { return m_actionHotkeySettings; }
    QAction* actionOpenConfigFolder() const { return m_actionOpenConfigFolder; }
    
    // Get all actions for easy access
    QList<QAction*> getAllActions() const;

private:
    MainWindow* m_parent;
    
    // Menus
    QMenu* m_menuProgram = nullptr;
    QMenu* m_menuPreferences = nullptr;
    QMenu* m_menuServer = nullptr;
    QMenu* m_menuSpmode = nullptr;
    QMenu* m_menuProgramPreference = nullptr;
    QMenu* m_menuActiveServer = nullptr;
    QMenu* m_menuActiveRouting = nullptr;
    QMenu* m_menuShareItem = nullptr;
    QMenu* m_menuCurrentGroup = nullptr;
    QMenu* m_menuCurrentSelect = nullptr;
    
    // Actions - Program menu
    QAction* m_actionShowWindow = nullptr;
    QAction* m_actionExit = nullptr;
    QAction* m_actionRestartProxy = nullptr;
    QAction* m_actionRestartProgram = nullptr;
    QAction* m_actionStartWithSystem = nullptr;
    QAction* m_actionRememberLastProxy = nullptr;
    QAction* m_actionAllowLAN = nullptr;
    QAction* m_actionPlaceholder1 = nullptr;
    QAction* m_actionPlaceholder2 = nullptr;
    QAction* m_actionPlaceholder3 = nullptr;
    
    // Actions - System Proxy
    QAction* m_actionSpmodeSystemProxy = nullptr;
    QAction* m_actionSpmodeVPN = nullptr;
    QAction* m_actionSpmodeDisabled = nullptr;
    
    // Actions - Server menu
    QAction* m_actionStart = nullptr;
    QAction* m_actionStop = nullptr;
    QAction* m_actionAddFromInput = nullptr;
    QAction* m_actionAddFromClipboard = nullptr;
    QAction* m_actionSelectAll = nullptr;
    QAction* m_actionMove = nullptr;
    QAction* m_actionClone = nullptr;
    QAction* m_actionResetTraffic = nullptr;
    QAction* m_actionDelete = nullptr;
    QAction* m_actionQR = nullptr;
    QAction* m_actionExportConfig = nullptr;
    QAction* m_actionCopyLinks = nullptr;
    QAction* m_actionCopyLinksNKR = nullptr;
    QAction* m_actionTCPPing = nullptr;
    QAction* m_actionURLTest = nullptr;
    QAction* m_actionFullTest = nullptr;
    QAction* m_actionStopTesting = nullptr;
    QAction* m_actionClearTestResult = nullptr;
    QAction* m_actionResolveDomain = nullptr;
    QAction* m_actionRemoveUnavailable = nullptr;
    QAction* m_actionDeleteRepeat = nullptr;
    QAction* m_actionUpdateSubscription = nullptr;
    QAction* m_actionProfileDebugInfo = nullptr;
    QAction* m_actionAddFromClipboard2 = nullptr;
    QAction* m_actionScanQR = nullptr;
    
    // Actions - Preferences menu
    QAction* m_actionManageGroups = nullptr;
    QAction* m_actionBasicSettings = nullptr;
    QAction* m_actionRoutingSettings = nullptr;
    QAction* m_actionVPNSettings = nullptr;
    QAction* m_actionHotkeySettings = nullptr;
    QAction* m_actionOpenConfigFolder = nullptr;
    
    // Action groups
    QActionGroup* m_spmodeGroup = nullptr;
    
    // Helper methods
    void buildProgramMenu();
    void buildPreferencesMenu();
    void buildServerMenu();
    QAction* createAction(const QString& text, const QString& shortcut = "", bool checkable = false);
    QAction* createPlaceholderAction();
};
