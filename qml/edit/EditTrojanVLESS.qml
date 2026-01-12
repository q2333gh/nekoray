import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GroupBox {
    title: qsTr("Trojan/VLESS Settings")
    
    property var controller: null
    
    function setController(ctrl) {
        controller = ctrl
    }
    
    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 10
        rowSpacing: 10

        Label { text: qsTr("Password/UUID:") }
        TextField {
            id: passwordField
            placeholderText: qsTr("Enter password or UUID")
            Layout.fillWidth: true
        }

        Label { text: qsTr("Flow:") }
        ComboBox {
            id: flowField
            model: ["", "xtls-rprx-vision", "xtls-rprx-vision-udp443"]
            Layout.fillWidth: true
        }
    }
}
