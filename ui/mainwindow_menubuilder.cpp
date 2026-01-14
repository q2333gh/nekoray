#include "mainwindow_menubuilder.h"
#include "mainwindow.h"
#include <QKeySequence>

MenuBuilder::MenuBuilder(MainWindow* parent) : m_parent(parent) {
    m_spmodeGroup = new QActionGroup(parent);
    m_spmodeGroup->setExclusive(true);
}

void MenuBuilder::buildMenus(QMenuBar* menubar) {
    buildProgramMenu();
    buildPreferencesMenu();
    buildServerMenu();
    
    menubar->addMenu(m_menuProgram);
    menubar->addMenu(m_menuPreferences);
    menubar->addMenu(m_menuServer);
}

void MenuBuilder::buildProgramMenu() {
    m_menuProgram = new QMenu(QObject::tr("Program"), m_parent);
    
    // System Proxy submenu
    m_menuSpmode = new QMenu(QObject::tr("System Proxy"), m_parent);
    m_actionSpmodeSystemProxy = createAction(QObject::tr("Enable System Proxy"), "", true);
    m_actionSpmodeVPN = createAction(QObject::tr("Enable Tun"), "", true);
    m_actionSpmodeDisabled = createAction(QObject::tr("Disable"), "", true);
    
    m_actionSpmodeSystemProxy->setActionGroup(m_spmodeGroup);
    m_actionSpmodeVPN->setActionGroup(m_spmodeGroup);
    m_actionSpmodeDisabled->setActionGroup(m_spmodeGroup);
    
    m_menuSpmode->addAction(m_actionSpmodeSystemProxy);
    m_menuSpmode->addAction(m_actionSpmodeVPN);
    m_menuSpmode->addAction(m_actionSpmodeDisabled);
    
    // Preferences submenu (will be populated with menu_preferences actions)
    m_menuProgramPreference = new QMenu(QObject::tr("Preferences"), m_parent);
    
    // Active Server submenu (populated dynamically)
    m_menuActiveServer = new QMenu(QObject::tr("Active Server"), m_parent);
    
    // Active Routing submenu (populated dynamically)
    m_menuActiveRouting = new QMenu(QObject::tr("Active Routing"), m_parent);
    
    // Main program menu items
    m_actionShowWindow = createAction(QObject::tr("Show Window"));
    m_actionAddFromClipboard2 = createAction(QObject::tr("Add profile from clipboard"));
    m_actionScanQR = createAction(QObject::tr("Scan QR Code"));
    
    m_menuProgram->addAction(m_actionShowWindow);
    m_menuProgram->addAction(m_actionAddFromClipboard2);
    m_menuProgram->addAction(m_actionScanQR);
    m_menuProgram->addSeparator();
    
    m_actionStartWithSystem = createAction(QObject::tr("Start with system"), "", true);
    m_actionRememberLastProxy = createAction(QObject::tr("Remember last profile"), "", true);
    m_actionAllowLAN = createAction(QObject::tr("Allow other devices to connect"), "", true);
    
    m_menuProgram->addAction(m_actionStartWithSystem);
    m_menuProgram->addAction(m_actionRememberLastProxy);
    m_menuProgram->addAction(m_actionAllowLAN);
    m_menuProgram->addMenu(m_menuActiveServer);
    m_menuProgram->addMenu(m_menuActiveRouting);
    m_menuProgram->addMenu(m_menuSpmode);
    m_menuProgram->addMenu(m_menuProgramPreference);
    
    m_menuProgram->addSeparator();
    
    m_actionRestartProxy = createAction(QObject::tr("Restart Proxy"));
    m_actionRestartProgram = createAction(QObject::tr("Restart Program"));
    m_actionExit = createAction(QObject::tr("Exit"));
    
    m_menuProgram->addAction(m_actionRestartProxy);
    m_menuProgram->addAction(m_actionRestartProgram);
    m_menuProgram->addAction(m_actionExit);
    
    // Add placeholder for Windows tray menu bug fix
    m_actionPlaceholder1 = createPlaceholderAction();
    m_menuProgram->addAction(m_actionPlaceholder1);
}

void MenuBuilder::buildPreferencesMenu() {
    m_menuPreferences = new QMenu(QObject::tr("Preferences"), m_parent);
    
    m_actionManageGroups = createAction(QObject::tr("Groups"));
    m_actionBasicSettings = createAction(QObject::tr("Basic Settings"));
    m_actionRoutingSettings = createAction(QObject::tr("Routing Settings"));
    m_actionVPNSettings = createAction(QObject::tr("Tun Settings"));
    m_actionHotkeySettings = createAction(QObject::tr("Hotkey Settings"));
    m_actionOpenConfigFolder = createAction(QObject::tr("Open Config Folder"));
    
    m_menuPreferences->addAction(m_actionManageGroups);
    m_menuPreferences->addAction(m_actionBasicSettings);
    m_menuPreferences->addAction(m_actionRoutingSettings);
    m_menuPreferences->addAction(m_actionVPNSettings);
    m_menuPreferences->addAction(m_actionHotkeySettings);
    m_menuPreferences->addAction(m_actionOpenConfigFolder);
    
    // Also add to program preference submenu
    m_menuProgramPreference->addActions(m_menuPreferences->actions());
}

void MenuBuilder::buildServerMenu() {
    m_menuServer = new QMenu(QObject::tr("Server"), m_parent);
    
    // Share submenu
    m_menuShareItem = new QMenu(QObject::tr("Share"), m_parent);
    m_actionQR = createAction(QObject::tr("QR Code and link"), "Ctrl+Q");
    m_actionExportConfig = createAction(QObject::tr("Export %1 config"), "Ctrl+E");
    m_actionCopyLinks = createAction(QObject::tr("Copy links of selected"), "Ctrl+C");
    m_actionCopyLinksNKR = createAction(QObject::tr("Copy links of selected (Neko Links)"), "Ctrl+N");
    
    m_menuShareItem->addAction(m_actionQR);
    m_menuShareItem->addAction(m_actionExportConfig);
    m_menuShareItem->addSeparator();
    m_menuShareItem->addAction(m_actionCopyLinks);
    m_menuShareItem->addAction(m_actionCopyLinksNKR);
    
    // Current Group submenu
    m_menuCurrentGroup = new QMenu(QObject::tr("Current Group"), m_parent);
    m_actionTCPPing = createAction(QObject::tr("Tcp Ping"), "Ctrl+Alt+T");
    m_actionURLTest = createAction(QObject::tr("Url Test"), "Ctrl+Alt+U");
    m_actionFullTest = createAction(QObject::tr("Full Test"), "Ctrl+Alt+F");
    m_actionStopTesting = createAction(QObject::tr("Stop Testing"), "Ctrl+Alt+S");
    m_actionClearTestResult = createAction(QObject::tr("Clear Test Result"), "Ctrl+Alt+C");
    m_actionResolveDomain = createAction(QObject::tr("Resolve domain"), "Ctrl+Alt+I");
    m_actionRemoveUnavailable = createAction(QObject::tr("Remove Unavailable"), "Ctrl+Alt+R");
    m_actionDeleteRepeat = createAction(QObject::tr("Remove Duplicates"), "Ctrl+Alt+D");
    m_actionUpdateSubscription = createAction(QObject::tr("Update subscription"), "Ctrl+U");
    
    m_menuCurrentGroup->addAction(m_actionTCPPing);
    m_menuCurrentGroup->addAction(m_actionURLTest);
    m_menuCurrentGroup->addAction(m_actionFullTest);
    m_menuCurrentGroup->addAction(m_actionStopTesting);
    m_menuCurrentGroup->addAction(m_actionClearTestResult);
    m_menuCurrentGroup->addAction(m_actionResolveDomain);
    m_menuCurrentGroup->addSeparator();
    m_menuCurrentGroup->addAction(m_actionRemoveUnavailable);
    m_menuCurrentGroup->addAction(m_actionDeleteRepeat);
    m_menuCurrentGroup->addSeparator();
    m_menuCurrentGroup->addAction(m_actionUpdateSubscription);
    
    // Current Select submenu (populated dynamically)
    m_menuCurrentSelect = new QMenu(QObject::tr("Current Select"), m_parent);
    
    // Main server menu items
    m_actionAddFromInput = createAction(QObject::tr("New profile"));
    m_actionAddFromClipboard = createAction(QObject::tr("Add profile from clipboard"), "Ctrl+V");
    m_actionStart = createAction(QObject::tr("Start"), "Return");
    m_actionStop = createAction(QObject::tr("Stop"), "Ctrl+S");
    m_actionSelectAll = createAction(QObject::tr("Select All"), "Ctrl+A");
    m_actionMove = createAction(QObject::tr("Move"), "Ctrl+M");
    m_actionClone = createAction(QObject::tr("Clone"), "Ctrl+D");
    m_actionResetTraffic = createAction(QObject::tr("Reset Traffic"), "Ctrl+R");
    m_actionDelete = createAction(QObject::tr("Delete"), "Del");
    m_actionProfileDebugInfo = createAction(QObject::tr("Debug Info"));
    
    m_menuServer->addAction(m_actionAddFromInput);
    m_menuServer->addAction(m_actionAddFromClipboard);
    m_menuServer->addSeparator();
    m_menuServer->addAction(m_actionStart);
    m_menuServer->addAction(m_actionStop);
    m_menuServer->addSeparator();
    m_menuServer->addAction(m_actionSelectAll);
    m_menuServer->addAction(m_actionMove);
    m_menuServer->addAction(m_actionClone);
    m_menuServer->addAction(m_actionResetTraffic);
    m_menuServer->addAction(m_actionDelete);
    m_menuServer->addSeparator();
    m_menuServer->addMenu(m_menuShareItem);
    m_menuServer->addSeparator();
    m_menuServer->addMenu(m_menuCurrentSelect);
    m_menuServer->addMenu(m_menuCurrentGroup);
    m_menuServer->addSeparator();
    m_menuServer->addAction(m_actionProfileDebugInfo);
    m_menuServer->addSeparator();
}

QAction* MenuBuilder::createAction(const QString& text, const QString& shortcut, bool checkable) {
    auto action = new QAction(text, m_parent);
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    action->setCheckable(checkable);
    return action;
}

QAction* MenuBuilder::createPlaceholderAction() {
    auto action = new QAction(" ", m_parent);
    action->setEnabled(false);
    return action;
}

QList<QAction*> MenuBuilder::getAllActions() const {
    QList<QAction*> actions;
    if (m_actionShowWindow) actions << m_actionShowWindow;
    if (m_actionExit) actions << m_actionExit;
    if (m_actionStart) actions << m_actionStart;
    if (m_actionStop) actions << m_actionStop;
    if (m_actionAddFromClipboard) actions << m_actionAddFromClipboard;
    if (m_actionAddFromClipboard2) actions << m_actionAddFromClipboard2;
    if (m_actionAddFromInput) actions << m_actionAddFromInput;
    if (m_actionStartWithSystem) actions << m_actionStartWithSystem;
    if (m_actionRememberLastProxy) actions << m_actionRememberLastProxy;
    if (m_actionAllowLAN) actions << m_actionAllowLAN;
    if (m_actionRestartProxy) actions << m_actionRestartProxy;
    if (m_actionRestartProgram) actions << m_actionRestartProgram;
    if (m_actionSpmodeSystemProxy) actions << m_actionSpmodeSystemProxy;
    if (m_actionSpmodeVPN) actions << m_actionSpmodeVPN;
    if (m_actionSpmodeDisabled) actions << m_actionSpmodeDisabled;
    if (m_actionSelectAll) actions << m_actionSelectAll;
    if (m_actionMove) actions << m_actionMove;
    if (m_actionClone) actions << m_actionClone;
    if (m_actionResetTraffic) actions << m_actionResetTraffic;
    if (m_actionDelete) actions << m_actionDelete;
    if (m_actionQR) actions << m_actionQR;
    if (m_actionExportConfig) actions << m_actionExportConfig;
    if (m_actionCopyLinks) actions << m_actionCopyLinks;
    if (m_actionCopyLinksNKR) actions << m_actionCopyLinksNKR;
    if (m_actionTCPPing) actions << m_actionTCPPing;
    if (m_actionURLTest) actions << m_actionURLTest;
    if (m_actionFullTest) actions << m_actionFullTest;
    if (m_actionStopTesting) actions << m_actionStopTesting;
    if (m_actionClearTestResult) actions << m_actionClearTestResult;
    if (m_actionResolveDomain) actions << m_actionResolveDomain;
    if (m_actionRemoveUnavailable) actions << m_actionRemoveUnavailable;
    if (m_actionDeleteRepeat) actions << m_actionDeleteRepeat;
    if (m_actionUpdateSubscription) actions << m_actionUpdateSubscription;
    return actions;
}
