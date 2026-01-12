import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import Nekoray 1.0
import "components"
import "dialogs"
import "edit"

ApplicationWindow {
    id: window
    visible: true
    width: 800
    height: 600
    title: qsTr("Nekoray")

    property MainWindowController controller: mainController

    // Dialogs
    BasicSettingsDialog {
        id: basicSettingsDialog
    }

    EditProfileDialog {
        id: editProfileDialog
    }

    Connections {
        target: controller
        function onEditProfileRequested(profileId) {
            editProfileDialog.openForEdit(profileId)
        }
        function onNewProfileRequested(type, groupId) {
            editProfileDialog.openForNew(type, groupId)
        }
    }

    // System Tray
    SystemTrayIcon {
        id: systemTray
        visible: dataStore.flag_tray
        icon.source: "qrc:/neko/nekobox.png"
        tooltip: qsTr("Nekoray")

        menu: Menu {
            MenuItem {
                text: controller.isRunning ? qsTr("Stop") : qsTr("Start")
                onTriggered: {
                    if (controller.isRunning) {
                        controller.stop()
                    } else {
                        controller.start()
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Show Window")
                onTriggered: window.show()
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit()
            }
        }

        onActivated: {
            if (reason === SystemTrayIcon.Trigger) {
                window.show()
                window.raise()
                window.requestActivate()
            }
        }
    }

    // Shortcuts
    Shortcut {
        sequence: "Return"
        onActivated: {
            if (controller.isRunning) {
                controller.stop()
            } else {
                controller.start()
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: controller.stop()
    }

    Shortcut {
        sequence: "Ctrl+F"
        // TODO: Focus search/filter
    }

    Shortcut {
        sequence: "Esc"
        // TODO: Clear selection
    }

    // Window state handling
    onClosing: {
        if (systemTray.visible) {
            close.accepted = false
            window.hide()
        } else {
            controller.commitDataRequest()
        }
    }

    // Status update timer
    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: controller.refreshStatus()
    }

    // Auto-hide if tray mode
    Component.onCompleted: {
        if (dataStore.flag_tray && dataStore.start_minimal) {
            window.hide()
        }
    }

    // Top toolbar
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 10

            ToolButton {
                text: qsTr("Program")
                Menu {
                    MenuItem {
                        text: qsTr("Show Window")
                        onTriggered: window.show()
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Start with system")
                        checkable: true
                        checked: false // TODO: Bind to dataStore
                    }
                    MenuItem {
                        text: qsTr("Remember last profile")
                        checkable: true
                        checked: false // TODO: Bind to dataStore
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Restart Proxy")
                        onTriggered: {
                            controller.stop()
                            Qt.callLater(() => controller.start())
                        }
                    }
                    MenuItem {
                        text: qsTr("Restart Program")
                        onTriggered: Qt.quit() // TODO: Implement restart
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Exit")
                        onTriggered: Qt.quit()
                    }
                }
            }

            ToolButton {
                text: qsTr("Preferences")
                onClicked: {
                    basicSettingsDialog.open()
                }
            }

            ToolButton {
                text: qsTr("Server")
                Menu {
                    MenuItem {
                        text: qsTr("New profile")
                        onTriggered: {
                            editProfileDialog.openForNew("socks", controller.currentGroupId)
                        }
                    }
                    MenuItem {
                        text: qsTr("Add from clipboard")
                        onTriggered: controller.addProfileFromClipboard()
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Start")
                        shortcut: "Return"
                        enabled: !controller.isRunning
                        onTriggered: controller.start()
                    }
                    MenuItem {
                        text: qsTr("Stop")
                        shortcut: "Ctrl+S"
                        enabled: controller.isRunning
                        onTriggered: controller.stop()
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Clone")
                        shortcut: "Ctrl+D"
                        onTriggered: {
                            // TODO: Get selected profile
                        }
                    }
                    MenuItem {
                        text: qsTr("Delete")
                        shortcut: "Del"
                        onTriggered: {
                            // TODO: Get selected profile
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                text: qsTr("Document")
                onClicked: Qt.openUrlExternally("https://matsuridayo.github.io/")
            }

            ToolButton {
                text: qsTr("Update")
                onClicked: {
                    // TODO: Check for updates
                }
            }

            ToolButton {
                text: qsTr("Test")
                onClicked: controller.speedtestCurrentGroup()
            }
        }
    }

    // Main content
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Left: Group tabs and proxy list
        ColumnLayout {
            Layout.minimumWidth: 400
            Layout.fillWidth: true

            // Group tabs
            TabBar {
                id: groupTabBar
                Layout.fillWidth: true
                currentIndex: 0

                Repeater {
                    model: controller.groupModel
                    TabButton {
                        text: model.name || qsTr("Group %1").arg(model.groupId)
                        onClicked: {
                            controller.showGroup(model.groupId)
                        }
                    }
                }
            }

            // Proxy list with drag and drop
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: proxyListView
                    model: controller.profileModel
                    spacing: 2
                    clip: true

                    delegate: ProxyItem {
                        id: proxyItemDelegate
                        width: proxyListView.width
                    }

                    DropArea {
                        anchors.fill: parent
                        onEntered: {
                            if (drag.source) {
                                drag.accept()
                            }
                        }
                        onDropped: {
                            // Handle drop to reorder profiles
                            // TODO: Implement reorder logic
                        }
                    }
                }
            }
        }

        // Right: Log and connection info
        ColumnLayout {
            Layout.minimumWidth: 300
            Layout.fillWidth: true

            TabBar {
                id: contentTabBar
                Layout.fillWidth: true

                TabButton {
                    text: qsTr("Log")
                }
                TabButton {
                    text: qsTr("Connections")
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: contentTabBar.currentIndex

                // Log page
                ScrollView {
                    TextArea {
                        id: logTextArea
                        readOnly: true
                        text: controller.logText
                        font.family: "Consolas, Courier New, monospace"
                        font.pixelSize: 9
                        wrapMode: TextArea.Wrap

                        Connections {
                            target: controller
                            function onLogTextChanged() {
                                logTextArea.text = controller.logText
                                // Auto-scroll to bottom
                                logTextArea.cursorPosition = logTextArea.length
                            }
                        }
                    }
                }

                // Connections page
                ListView {
                    model: ListModel {
                        // TODO: Populate from connection list
                    }
                    delegate: Text {
                        text: modelData || "No connections"
                    }
                }
            }
        }
    }

}
