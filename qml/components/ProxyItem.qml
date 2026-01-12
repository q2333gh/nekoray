import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: proxyItem
    width: ListView.view ? ListView.view.width : 200
    height: 60
    color: model.isRunning ? "#E8F5E9" : (mouseArea.containsMouse ? "#F5F5F5" : "white")
    border.color: model.isRunning ? "#4CAF50" : "#E0E0E0"
    border.width: model.isRunning ? 2 : 1
    radius: 4

    property bool isSelected: false

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 10

        // Drag handle
        Rectangle {
            Layout.preferredWidth: 4
            Layout.preferredHeight: parent.height - 16
            color: "#CCCCCC"
            radius: 2
            visible: mouseArea.containsMouse

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeAllCursor
                drag.target: proxyItem
                drag.axis: Drag.YAxis
                onPressed: {
                    proxyItem.Drag.active = true
                }
                onReleased: {
                    proxyItem.Drag.drop()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Text {
                    text: model.name || qsTr("Profile %1").arg(model.proxyId)
                    font.bold: true
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: model.latencyColor || "#666666"
                }

                Text {
                    text: model.latencyDisplay || "-"
                    font.pixelSize: 11
                    color: model.latencyColor || "#666666"
                }
            }

            Text {
                text: model.type + " - " + model.address + ":" + model.port
                font.pixelSize: 12
                color: "#666666"
                elide: Text.ElideRight
            }

            Text {
                text: model.traffic || ""
                font.pixelSize: 11
                color: "#999999"
                visible: text !== ""
            }
        }

        Button {
            text: model.isRunning ? qsTr("Stop") : qsTr("Start")
            highlighted: model.isRunning
            onClicked: {
                if (model.isRunning) {
                    mainController.stop()
                } else {
                    mainController.start(model.proxyId)
                }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.LeftButton) {
                if (mouse.modifiers & Qt.ControlModifier) {
                    // Multi-select
                    isSelected = !isSelected
                } else {
                    mainController.start(model.proxyId)
                }
            } else if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            }
        }
        onDoubleClicked: {
            mainController.start(model.proxyId)
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Start")
            enabled: !model.isRunning
            onTriggered: mainController.start(model.proxyId)
        }
        MenuItem {
            text: qsTr("Stop")
            enabled: model.isRunning
            onTriggered: mainController.stop()
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Edit")
            onTriggered: {
                mainController.showEditProfile(model.proxyId)
            }
        }
        MenuItem {
            text: qsTr("Clone")
            onTriggered: mainController.cloneProfile(model.proxyId)
        }
        MenuItem {
            text: qsTr("Delete")
            onTriggered: mainController.deleteProfile(model.proxyId)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Reset Traffic")
            onTriggered: mainController.resetTraffic(model.proxyId)
        }
    }

    Drag.active: mouseArea.drag.active
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2

    states: [
        State {
            when: proxyItem.Drag.active
            PropertyChanges {
                target: proxyItem
                opacity: 0.5
            }
        }
    ]
}
