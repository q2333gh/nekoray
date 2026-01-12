import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GroupBox {
    title: qsTr("VMess Settings")
    
    property var controller: null
    
    function setController(ctrl) {
        controller = ctrl
        // Load VMess-specific data
        if (controller && controller.profileId >= 0) {
            loadVMessData()
        }
    }
    
    function loadVMessData() {
        // TODO: Load VMess bean data
    }
    
    function saveVMessData() {
        // TODO: Save VMess bean data
    }
    
    GridLayout {
        anchors.fill: parent
        columns: 2
        columnSpacing: 10
        rowSpacing: 10

        Label { text: qsTr("UUID:") }
        RowLayout {
            TextField {
                id: uuidField
                placeholderText: qsTr("Enter UUID")
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Generate")
                onClicked: {
                    // Generate UUID
                    uuidField.text = Qt.createQmlObject('import QtQuick 2.15; QtObject {}', parent).toString()
                    // TODO: Use QUuid from C++
                }
            }
        }

        Label { text: qsTr("Alter Id:") }
        SpinBox {
            id: aidField
            from: 0
            to: 65535
            value: 0
        }

        Label { text: qsTr("Security:") }
        ComboBox {
            id: securityField
            model: ["auto", "zero", "none", "chacha20-poly1305", "aes-128-gcm"]
            currentIndex: 0
        }
    }
}
