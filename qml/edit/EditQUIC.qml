import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GroupBox {
    title: qsTr("QUIC Settings (Hysteria2/TUIC)")
    
    property var controller: null
    
    function setController(ctrl) {
        controller = ctrl
    }
    
    ScrollView {
        anchors.fill: parent
        
        GridLayout {
            width: parent.width
            columns: 2
            columnSpacing: 10
            rowSpacing: 10

            Label { text: qsTr("Password:") }
            TextField {
                id: passwordField
                Layout.fillWidth: true
            }

            Label { text: qsTr("UUID:") }
            RowLayout {
                TextField {
                    id: uuidField
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Generate")
                    // TODO: Generate UUID
                }
            }

            Label { text: qsTr("SNI:") }
            TextField {
                id: sniField
                Layout.fillWidth: true
            }

            Label { text: qsTr("Upload Mbps:") }
            SpinBox {
                id: uploadMbpsField
                from: 0
                to: 10000
                Layout.fillWidth: true
            }

            Label { text: qsTr("Download Mbps:") }
            SpinBox {
                id: downloadMbpsField
                from: 0
                to: 10000
                Layout.fillWidth: true
            }
        }
    }
}
