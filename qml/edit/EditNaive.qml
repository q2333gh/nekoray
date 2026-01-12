import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GroupBox {
    title: qsTr("Naive Settings")
    
    property var controller: null
    
    function setController(ctrl) {
        controller = ctrl
    }
    
    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 10
        rowSpacing: 10

        Label { text: qsTr("Username:") }
        TextField {
            id: usernameField
            Layout.fillWidth: true
        }

        Label { text: qsTr("Password:") }
        TextField {
            id: passwordField
            echoMode: TextField.Password
            Layout.fillWidth: true
        }

        Label { text: qsTr("Protocol:") }
        ComboBox {
            id: protocolField
            model: ["", "https", "quic"]
            Layout.fillWidth: true
        }

        Label { text: qsTr("SNI:") }
        TextField {
            id: sniField
            Layout.fillWidth: true
        }
    }
}
