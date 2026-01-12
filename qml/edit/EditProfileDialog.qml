import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Nekoray 1.0

Dialog {
    id: dialog
    title: qsTr("Edit Profile")
    width: 1000
    height: 700
    modal: true

    property EditProfileController controller: EditProfileController {
        id: editController
        onSaved: {
            dialog.close()
            mainController.refreshProxyList()
        }
    }

    property int editingProfileId: -1
    property string editingType: ""
    property int editingGroupId: -1

    function openForEdit(profileId) {
        editingProfileId = profileId
        editController.loadProfile(profileId)
        dialog.open()
    }

    function openForNew(type, groupId) {
        editingProfileId = -1
        editingType = type
        editingGroupId = groupId
        editController.createNew(type, groupId)
        dialog.open()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 10

        // Left: Common settings
        ScrollView {
            Layout.preferredWidth: 400
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                GroupBox {
                    title: qsTr("Common")
                    Layout.fillWidth: true

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 10

                        Label { text: qsTr("Type:") }
                        ComboBox {
                            id: typeCombo
                            model: ["socks", "http", "shadowsocks", "trojan", "vmess", "vless", "naive", "hysteria2", "tuic", "chain"]
                            currentIndex: model.indexOf(editController.profileType)
                            enabled: editingProfileId < 0
                            Layout.fillWidth: true
                            onCurrentTextChanged: {
                                if (enabled) {
                                    editController.profileType = currentText
                                }
                            }
                        }

                        Label { text: qsTr("Name:") }
                        TextField {
                            text: editController.profileName
                            onTextChanged: editController.profileName = text
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Address:") }
                        TextField {
                            text: editController.serverAddress
                            onTextChanged: editController.serverAddress = text
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Port:") }
                        SpinBox {
                            value: editController.serverPort
                            onValueChanged: editController.serverPort = value
                            from: 1
                            to: 65535
                            Layout.fillWidth: true
                        }
                    }
                }

                // Protocol-specific settings
                Loader {
                    id: protocolLoader
                    Layout.fillWidth: true
                    source: {
                        switch (editController.profileType) {
                            case "vmess": return "qrc:/qml/edit/EditVMess.qml"
                            case "trojan":
                            case "vless": return "qrc:/qml/edit/EditTrojanVLESS.qml"
                            case "shadowsocks": return "qrc:/qml/edit/EditShadowSocks.qml"
                            case "socks":
                            case "http": return "qrc:/qml/edit/EditSocksHttp.qml"
                            case "naive": return "qrc:/qml/edit/EditNaive.qml"
                            case "hysteria2":
                            case "tuic": return "qrc:/qml/edit/EditQUIC.qml"
                            case "chain": return "qrc:/qml/edit/EditChain.qml"
                            default: return ""
                        }
                    }
                    onLoaded: {
                        if (item && typeof item.setController === "function") {
                            item.setController(editController)
                        }
                    }
                }
            }
        }

        // Right: Advanced settings (placeholder)
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 10

                GroupBox {
                    title: qsTr("Advanced Settings")
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Advanced settings for %1").arg(editController.profileType)
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

        onAccepted: {
            if (editController.save()) {
                dialog.close()
            }
        }
        onRejected: {
            dialog.close()
        }
    }
}
