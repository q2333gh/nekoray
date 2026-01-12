import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GroupBox {
    title: qsTr("Chain Proxy Settings")
    
    property var controller: null
    
    function setController(ctrl) {
        controller = ctrl
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: qsTr("Select profiles for chain:")
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel {
                // TODO: Populate from chain bean
            }
            delegate: Rectangle {
                width: ListView.view.width
                height: 40
                border.color: "#CCCCCC"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    Text {
                        text: modelData || qsTr("Profile")
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Change")
                        // TODO: Open profile selector
                    }
                    Button {
                        text: qsTr("Remove")
                        // TODO: Remove from chain
                    }
                }
            }
        }

        Button {
            text: qsTr("Add Profile")
            Layout.fillWidth: true
            // TODO: Open profile selector
        }
    }
}
