import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {
    id: settingsPage

    signal showError(string message)
    signal backRequested()
    signal saveAndRequestBack()

    background: Rectangle {
        color: Material.background
    }

    Label {
        id: settingsLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        font.pixelSize: 18
        text: qsTr("Settings")
    }

    ColumnLayout {
        anchors.top: settingsLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonRow.top
        anchors.margins: 20

        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton {
                text: qsTr("General")
            }
            TabButton {
                text: qsTr("Appearance")
            }
        }

        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 15
            currentIndex: tabBar.currentIndex

            Item {
                ScrollView {
                    anchors.fill: parent
                    contentWidth: availableWidth

                    Column {
                        padding: 10
                        width: parent.width
                        spacing: 10

                        Label {
                            text: qsTr("Language")
                            font.pixelSize: 16
                        }
                        ComboBox {
                            id: languageComboBox
                            currentIndex: settings.language === "ja" ? 0 : 1
                            model: ["日本語", "English"]
                        }
                    }
                }
            }

            Item {
                ScrollView {
                    anchors.fill: parent
                    contentWidth: availableWidth

                    Column {
                        padding: 10
                        width: parent.width
                        spacing: 10

                        Label {
                            text: qsTr("Theme")
                            font.pixelSize: 16
                        }
                        ComboBox {
                            id: themeComboBox
                            currentIndex: settings.theme === "light" ? 0 : 1
                            model: [qsTr("Light"), qsTr("Dark")]
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        id: buttonRow
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        spacing: 10

        Button {
            text: qsTr("Save")
            onClicked: {
                if (themeComboBox.currentIndex === 0) {
                    settings.setTheme("light")
                } else {
                    settings.setTheme("dark")
                }

                if (languageComboBox.currentIndex === 0) {
                    settings.setLanguage("ja")
                } else {
                    settings.setLanguage("en")
                }
                
                settingsPage.saveAndRequestBack()
            }
        }

        Button {
            text: qsTr("Back")
            onClicked: settingsPage.backRequested()
        }
    }
}