import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

/* global settings */

ApplicationWindow {
    id: root
    visible: true

    width: settings.windowWidth
    height: settings.windowHeight

    Material.theme: settings.theme === "light" ? Material.Light : Material.Dark

    title: qsTr("TitleMain")

    onClosing: function (close) {
        close.accepted = false;
        exitDialog.open();
    }

    header: ToolBar {
        RowLayout {
            anchors.fill : parent
            spacing: 10

            ToolButton {
                text: qsTr("ToolBarFile")

                Menu {
                    id: fileMenu
                    y: parent.height

                    MenuItem {
                        text: qsTr("ToolBarExit")
                        implicitWidth: 100
                        implicitHeight: 30
                        contentItem: Text {
                            text: parent.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            root.close();
                        }
                    }
                }

                onClicked: fileMenu.open()
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainContent
    }

    Component {
        id: mainContent

        Item {
            Label {
                anchors.centerIn: parent

                text: qsTr("Text1")
            }

            ComboBox {
                id: workspaceComboBox

                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 12
                anchors.bottomMargin: 12

                model: [qsTr("WorkspaceCreateNew"), qsTr("WorkSpaceOpen")]

                currentIndex: -1
                displayText: currentIndex === -1 ? qsTr("WorkSpaceSelect") : currentText

                property bool isInitialized: false

                Component.onCompleted: {
                    isInitialized = true
                }

                onCurrentIndexChanged: {

                    if (isInitialized && currentIndex !== -1) {
                        handleSelectionChange(currentIndex)
                        currentIndex = -1
                    }
                }
            }
        }
    }

    Component {
        id: createWorkspaceComponent

        Item {
            Label {
                anchors.centerIn: parent
                text: qsTr("Create New Workspace")
            }

            Button {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Back")
                onClicked: stackView.pop()
            }
        }
    }

    Component {
        id: openWorkspaceComponent

        Item {
            Label {
                anchors.centerIn: parent
                text: qsTr("Open Workspace")
            }

            Button {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Back")
                onClicked: stackView.pop()
            }
        }
    }

    function handleSelectionChange(index) {
        switch (index) {
            case 0:
                stackView.push(createWorkspaceComponent)
                break
            case 1:
                stackView.push(openWorkspaceComponent)
                break
            default:
                console.log("Unknown option selected")
        }
    }

    MessageDialog {
        id: exitDialog
        title: qsTr("MessageDialogConfirmExit")
        text: qsTr("MessageDialogText")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: Qt.quit()
    }
}