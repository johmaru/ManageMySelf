import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {
    id: createWorkspaceItem

    // 親コンポーネントに通知するためのシグナル
    signal showError(string message)
    signal backRequested()
    signal workspaceCreated(string userName, string name, string path)

    background: Rectangle {
        color: Material.background
    }

    Label {
        id: userNameLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 10
        font.pixelSize: 18
        text: qsTr("UserName")
    }

    TextField {
        id: userNameTextField
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: userNameLabel.bottom
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 10
        placeholderText: qsTr("Enter your name")
    }

    Label {
        id: createWorkspaceLabel
        anchors.top: userNameTextField.bottom
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
        font.pixelSize: 18
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
        onAccepted: {
            var path = folderDialog.selectedFolder.toString();
            if (Qt.platform.os === "windows" && path.startsWith('file:///')) {
                path = path.substring(8);
            }
            if (Qt.platform.os === "windows" && path.startsWith('/')) {
                path = path.substring(1);
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
                if (userNameTextField.text.trim() === "") {
                    createWorkspaceItem.showError(qsTr("User name cannot be empty"));
                    return;
                }

                if (workspaceNameTextField.text.trim() === "") {
                    createWorkspaceItem.showError(qsTr("Workspace name cannot be empty"));
                    return;
                }
                if (workspacePathTextField.text.trim() === "") {
                    createWorkspaceItem.showError(qsTr("Workspace path cannot be empty"));
                    return;
                }
                createWorkspaceItem.workspaceCreated(userNameTextField.text, workspaceNameTextField.text, workspacePathTextField.text);
            }
        }

        Button {
            text: qsTr("Back")
            onClicked: createWorkspaceItem.backRequested()
        }
    }
}