import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material
import ManageMySelf.GUI 1.0

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

    header: Loader {
        sourceComponent: stackView.currentItem ? stackView.currentItem.headerComponent : undefined
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainContent
    }

    Component {
        id: mainContent

        Item {
            property Component headerComponent: ToolBar {
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

                onActivated: function(index) {
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
            onWorkspaceCreated: function(userName,name, path) {
                
                let result = settings.createWorkspaceFromQml(userName,name, path);

                if (result === 0) {
                    console.log("Workspace created successfully");
                    stackView.pop();
                } else if (result === -1) {
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
        id: mainUserPageComponent

        MainUserPage {
            id: mainUserPageInstance
            themeSettings: settings.theme
            onShowError: function(message) {
                errorLabel.text = message;
                errorDialog.open();
            }
            onBackRequested: function() {
                stackView.pop();
            }
            onRequestGetUserName: function(path) {
                let userName = settings.getSettings(path);
                if (userName.length > 0) {
                    mainUserPageInstance.userName = userName[0];
                } else {
                    mainUserPageInstance.userName = "Unknown";
                }
            }
            onRequestCreateDiary: function(year, month, day, title, path) {
                let result = settings.createDiary(year, month, day, title, path);
                if (result === 0) {
                    console.log("Diary created successfully");
                    mainUserPageInstance.reloadMonthData();
                } else {
                    errorLabel.text = qsTr("Failed to create diary");
                    errorDialog.open();
                }
            }

            onRequestCreateStatus: function(path) {
                let result = settings.createStatus(path);
                if (result === 0) {
                    console.log("Status created successfully");
                    mainUserPageInstance.reloadMonthData();
                } else {
                    errorLabel.text = qsTr("Failed to create status");
                    errorDialog.open();
                }
            }

            onRequestMonthUserDiarySqlData: function(year, month, path) {
                let sqlData = settings.getMonthUserDiarySqlData(year, month, path);
                if (sqlData) {
                    console.log("Retrieved diary data for", year, month, ":", sqlData);
                    mainUserPageInstance.handleDiaryData(sqlData);
                } else {
                    errorLabel.text = qsTr("Failed to retrieve diary data");
                    stackView.pop();
                }
            }

            onRequestMonthUserStatusData: function(year, month, path) {
                let statusData = settings.getMonthUserStatusData(year, month, path);
                if (statusData) {
                    mainUserPageInstance.handleStatusData(statusData);
                } else {
                    errorLabel.text = qsTr("Failed to retrieve status data");
                    stackView.pop();
                }
            }

            onRequestNavigateMarkdownEditor: function(contentPath) {
                stackView.push(markdownEditorComponent, { markdownContentPath: contentPath });
            }

            onRequestNavigateMarkdownViewer: function(contentPath) {
                stackView.push(markdownViewerComponent, { markdownContentPath: contentPath });
            }

            Component {
                id: markdownEditorComponent
                
                MarkEditor {
                    id: markdownEditorPage
                    onBackRequested: function() {
                        stackView.pop();
                    }
                    onRequestLoadMarkdownFile: function(path) {
                        let content = settings.loadMarkdownFile(path);
                            markdownEditorPage.rawMarkdownContent = content;
                            markdownEditorPage.markdownContentPath = path;
                    }
                    onRequestWriteMarkdownFile: function(path, content) {
                        let result = settings.writeMarkdownFile(path, content);
                        if (result === 0) {
                            console.log("Markdown file saved successfully");
                        } else {
                            errorLabel.text = qsTr("Failed to save markdown file");
                            errorDialog.open();
                        }
                    }
                }
            }
            
            Component {
                id: markdownViewerComponent

                MarkDownViewer {
                    id: markdownViewerPage
                    onBackRequested: function() {
                        stackView.pop();
                    }
                    onRequestLoadMarkdownFile: function(path) {
                        let content = settings.loadMarkdownFile(path);
                            markdownViewerPage.updateMarkdownContent(content);
                    }
                }
            }

            onRequestStatusEditor: function(year, month, day, jsonString, path, mode) {
                stackView.push(statusEditorComponent, { year: year, month: month, day: day, statusJson: jsonString, workspacePath: path, mode: mode });
            }

            Component {
                id: statusEditorComponent

                StatusEditor {
                    id: statusEditorPage
                    onBackRequested: function() {
                        stackView.pop();
                    }
                    onRequestEditStatus: function(year, month, day, jsonString, path) {
                        let rc = settings.editStatus(year, month, day, jsonString, path);
                        if (rc === 0) {
                            mainUserPageInstance.reloadMonthData();
                        }
                    }
                }
            }
        }
    }

    Component {
        id: openWorkspaceComponent

        Item {
            property var workspaceModel: settings.getWorkspaces()

            ColumnLayout{
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10


                Label {
                    text: qsTr("Open Workspace")
                    font.pixelSize: 20
                    Layout.alignment: Qt.AlignHCenter
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ListView {
                        id: workspaceListView
                        model: workspaceModel
                        spacing: 5
                        delegate: Item {
                            width: parent.width
                            height: 40

                            RowLayout {
                                anchors.fill: parent
                                spacing: 10

                                Label {
                                    text: modelData
                                    Layout.fillWidth: true
                                    verticalAlignment: Label.AlignVCenter
                                }

                                Button {
                                    text: qsTr("Open")
                                    onClicked: {
                                       let workspace = settings.getWorkspaceWithName(modelData);
                                        console.log("Opening workspace:", workspace[1]);
                                        stackView.push(mainUserPageComponent, { workspacePath: workspace[1] });
                                    }
                                }

                                Button {
                                    text: qsTr("Delete")
                                    onClicked: {
                                        console.log("Deleting workspace:", modelData);
                                        let result = settings.deleteWorkspace(modelData);
                                        if (result === 0) {
                                            console.log("Workspace deleted successfully");
                                            workspaceListView.model = settings.getWorkspaces();
                                        } else {
                                            errorLabel.text = qsTr("Failed to delete workspace");
                                            errorDialog.open();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10

                    Button {
                        text: qsTr("Open from Folder")
                        onClicked: {
                            folderDialog.open();
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
                            console.log("Selected folder path:", path);
                        }
                    }

                    Button {
                        text: qsTr("Back")
                        onClicked: stackView.pop()
                    }
                }
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