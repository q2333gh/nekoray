import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GroupBox {
    title: qsTr("Shadowsocks Settings")
    
    property var controller: null
    
    function setController(ctrl) {
        controller = ctrl
    }
    
    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 10
        rowSpacing: 10

        Label { text: qsTr("Method:") }
        ComboBox {
            id: methodField
            model: ["2022-blake3-aes-128-gcm", "2022-blake3-aes-256-gcm", "2022-blake3-chacha20-poly1305", "aes-128-gcm", "aes-256-gcm", "chacha20-poly1305"]
            Layout.fillWidth: true
        }

        Label { text: qsTr("Password:") }
        TextField {
            id: passwordField
            echoMode: TextField.Password
            Layout.fillWidth: true
        }

        Label { text: qsTr("UDP over TCP:") }
        CheckBox {
            id: uotField
        }

        Label { text: qsTr("Plugin:") }
        ComboBox {
            id: pluginField
            model: ["", "v2ray-plugin", "obfs-local"]
            Layout.fillWidth: true
        }

        Label { text: qsTr("Plugin Options:") }
        TextField {
            id: pluginOptsField
            Layout.fillWidth: true
        }
    }
}
