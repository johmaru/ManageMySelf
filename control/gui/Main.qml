import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

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
                        text: qsTr("ToolBarSettings")
                        implicitWidth: 50
                        implicitHeight: 30
                        contentItem: Text {
                            text: parent.text
                            color: Material.foreground
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            stackView.push(settingsComponent);
                        }
                    }

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

                onActivated: {
                    handleSelectionChange(index)
                    currentIndex = -1
                }
            }
        }
    }

    Component {
        id: settingsComponent

        Settings {
            onShowError: function(message) {
                errorLabel.text = message;
                errorDialog.open();
            }
            onBackRequested: function() {
                stackView.pop();
            }
            onSaveAndRequestBack: function() {
                stackView.pop();
            }
        }
    }

    Component {
        id: createWorkspaceComponent
        CreateWorkspaceForm {
            onShowError: function(message) {
                errorLabel.text = message;
                errorDialog.open();
            }
            onBackRequested: function() {
                stackView.pop();
            }
            onWorkspaceCreated: function(name, path) {
                
                let result = settings.createWorkspaceFromQml(name, path);

                if (result === 0) {
                    console.log("Workspace created successfully");
                    stackView.pop();
                } else if (result === 1) {
                    errorLabel.text = "Exist an Folder";
                    errorDialog.open();
                } else {
                    errorLabel.text = "Failed to create workspace";
                    errorDialog.open();
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
                stackView.push(createWorkspaceComponent);
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

    Dialog {
        id: errorDialog
        title: qsTr("Error")
        modal: true
        standardButtons: Dialog.Ok
        anchors.centerIn: parent

        Label {
            id: errorLabel
            text: qsTr("An error occurred")
            horizontalAlignment: Text.AlignHCenter
        }

        onAccepted: {
            // Handle error dialog acceptance if needed
        }
    }
}