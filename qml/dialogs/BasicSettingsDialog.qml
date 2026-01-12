import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Nekoray 1.0

Dialog {
    id: dialog
    title: qsTr("Basic Settings")
    width: 700
    height: 600
    modal: true

    property BasicSettingsController controller: BasicSettingsController {
        id: settingsController
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton { text: qsTr("Common") }
            TabButton { text: qsTr("Style") }
            TabButton { text: qsTr("Advanced") }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Common tab
            ScrollView {
                ColumnLayout {
                    width: parent.width
                    spacing: 10

                    GroupBox {
                        title: qsTr("Inbound")
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                Label { text: qsTr("Listen Address:") }
                                TextField {
                                    text: settingsController.inboundAddress
                                    onTextChanged: settingsController.inboundAddress = text
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: qsTr("Auth")
                                    // TODO: Open auth dialog
                                }
                            }

                            RowLayout {
                                Label { text: qsTr("Mixed (SOCKS+HTTP) Port:") }
                                SpinBox {
                                    value: settingsController.inboundSocksPort
                                    onValueChanged: settingsController.inboundSocksPort = value
                                    from: 1
                                    to: 65535
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: qsTr("Testing")
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                Label { text: qsTr("Latency Test URL:") }
                                TextField {
                                    text: settingsController.testLatencyUrl
                                    onTextChanged: settingsController.testLatencyUrl = text
                                    Layout.fillWidth: true
                                }
                            }

                            RowLayout {
                                Label { text: qsTr("Download Test URL:") }
                                TextField {
                                    text: settingsController.testDownloadUrl
                                    onTextChanged: settingsController.testDownloadUrl = text
                                    Layout.fillWidth: true
                                }
                            }

                            RowLayout {
                                Label { text: qsTr("Concurrent:") }
                                SpinBox {
                                    value: settingsController.testConcurrent
                                    onValueChanged: settingsController.testConcurrent = value
                                    from: 1
                                    to: 100
                                }
                                Label { text: qsTr("Timeout:") }
                                SpinBox {
                                    value: settingsController.testDownloadTimeout
                                    onValueChanged: settingsController.testDownloadTimeout = value
                                    from: 1
                                    to: 60
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: qsTr("Logging")
                        Layout.fillWidth: true

                        RowLayout {
                            Label { text: qsTr("Log Level:") }
                            ComboBox {
                                model: ["trace", "debug", "info", "warn", "error", "fatal", "panic"]
                                currentIndex: model.indexOf(settingsController.logLevel)
                                onCurrentTextChanged: settingsController.logLevel = currentText
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            // Style tab
            ScrollView {
                ColumnLayout {
                    width: parent.width
                    spacing: 10

                    GroupBox {
                        title: qsTr("Theme")
                        Layout.fillWidth: true

                        RowLayout {
                            Label { text: qsTr("Theme:") }
                            ComboBox {
                                model: [
                                    { text: qsTr("System"), value: "0" },
                                    { text: "flatgray", value: "1" },
                                    { text: "lightblue", value: "2" },
                                    { text: "blacksoft", value: "3" }
                                ]
                                textRole: "text"
                                currentIndex: {
                                    for (let i = 0; i < model.length; i++) {
                                        if (model[i].value === settingsController.theme) return i
                                    }
                                    return 0
                                }
                                onCurrentIndexChanged: {
                                    if (currentIndex >= 0) {
                                        settingsController.theme = model[currentIndex].value
                                    }
                                }
                                Layout.fillWidth: true
                            }
                        }
                    }

                    GroupBox {
                        title: qsTr("Behavior")
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent

                            CheckBox {
                                text: qsTr("Start Minimized")
                                checked: settingsController.startMinimal
                                onCheckedChanged: settingsController.startMinimal = checked
                            }

                            CheckBox {
                                text: qsTr("Connection Statistics")
                                checked: settingsController.connectionStatistics
                                onCheckedChanged: settingsController.connectionStatistics = checked
                            }
                        }
                    }
                }
            }

            // Advanced tab
            ScrollView {
                ColumnLayout {
                    width: parent.width
                    spacing: 10

                    Label {
                        text: qsTr("Advanced settings")
                        Layout.fillWidth: true
                    }
                }
            }
        }

        DialogButtonBox {
            Layout.fillWidth: true
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel

            onAccepted: {
                settingsController.saveSettings()
                dialog.close()
            }
            onRejected: {
                settingsController.loadSettings()
                dialog.close()
            }
        }
    }
}
