import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs

/* global settings */

ApplicationWindow {
    id: root
    visible: true

    Component.onCompleted: {
        requestActivate()
    }

    property bool forceClose: false

    width: settings.windowWidth
    height: settings.windowHeight

    Material.theme: settings.theme === "light" ? Material.Light : Material.Dark

    title: qsTr("TitleMain")

    onClosing: function (close) {
        if (forceClose) {
            close.accepted = true;
            return;
        }
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
                            color: Material.foreground
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
                id: createWorkspaceLabel
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 20
                font.pixelSize: 18
                text: qsTr("WorkspaceName")
            }

            TextField {
                id: workspaceNameTextField

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: createWorkspaceLabel.bottom
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: 20
            }

            Label {
                id: workspacePathLabel
                anchors.top: workspaceNameTextField.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 10
                text: qsTr("WorkspacePath")
            }

            RowLayout {
                anchors.top: workspacePathLabel.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 10
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                TextField {
                    id: workspacePathTextField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Select workspace path")
                    readOnly: true
                }

                Button {
                    text: "..."
                    onClicked: folderDialog.open()
                }
            }

            FolderDialog {
                id: folderDialog
                title: qsTr("Select Workspace Folder")
                selectedFolder: workspacePathTextField.text
                onAccepted: {
                    var path = folderDialog.selectedFolder.toString();
                    if (Qt.platform.os === "windows" && path.startsWith('/')) {
                        path = path.substring(1)
                    }

                    if (Qt.platform.os === "windows" && path.startsWith('file:///')) {
                        path = path.substring(8) // Remove 'file:///' prefix
                    }

                    workspacePathTextField.text = path;
                }
            }

             RowLayout {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 20
                spacing: 10

                Button {
                    text: qsTr("Create")
                    onClicked: {
                        console.log("Create workspace: " + workspaceNameTextField.text)
                        console.log("Workspace path: " + workspacePathTextField.text)
                        stackView.pop()
                    }
                }

                Button {
                    text: qsTr("Back")
                    onClicked: stackView.pop()
                }
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

    Dialog {
        id: exitDialog
        title: qsTr("MessageDialogConfirmExit")
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        anchors.centerIn: parent

        Label {
            text: qsTr("MessageDialogText")
            horizontalAlignment: Text.AlignHCenter
        }

        onAccepted: {
            forceClose = true;
            root.close();
        }
    }
}