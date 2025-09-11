pragma ComponentBehavior: Bound
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
    Material.primary: Material.Blue
    Material.accent: Material.Blue

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
            id: mainContentItem
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

            function handleActivityChange(key) {
                    switch (key) {
                        case "toggle":
                            sidePanel.isSelected = !sidePanel.isSelected
                            break;
                        case "createWorkspace":
                            stackView.push(createWorkspaceComponent);
                            break;
                        case "openWorkspace":
                            stackView.push(openWorkspaceComponent);
                            break;
                        case "settings":
                            if (sidePanel.isSelected) sidePanel.isSelected = false
                            else sidePanel.isSelected = true
                            break;
                    }
            }

            StackView.onStatusChanged: {
                if (StackView.status === StackView.Active) {
                    activityLoader.currentKey = ""
                } 
            }

            RowLayout {
                anchors.fill: parent
                spacing: 0

                ActivityBar {
                    id: activityLoader
                    Layout.preferredWidth: 120
                    Layout.fillHeight: true
                    mode: 0
                    scene: ActivityBar.Scene.Main
                    Material.theme: root.Material.theme
                    Material.primary: root.Material.primary
                    Material.accent: root.Material.accent
                    theme: settings.theme
                    // onCurrentIndexChanged: statusEditorPage.handleActivityChange(currentIndex)
                    onActivated: function(key) { mainContentItem.handleActivityChange(key) }
                }

                // サイドパネル
                Frame {
                    id: sidePanel
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true

                    property bool isSelected: false

                    visible: isSelected

                    StackLayout {
                        id: sideStack
                        anchors.fill: parent
                        currentIndex: activityLoader.currentKey === "settings" ? 1 : 0

                        Column {
                            spacing: 8
                            padding: 8
                        }

                        Column {
                            spacing: 8
                            padding: 8
                            Label { text: qsTr("Settings") }
                            Label { text: qsTr("Currently does not support this feature") }
                        }
                    }
                }

                Pane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    padding: 0
                    topPadding: 0
                    bottomPadding: 0
                    leftPadding: 0
                    rightPadding: 0

                    Loader {
                        id: mainContentLoader
                        anchors.fill: parent
                        sourceComponent: mainMenuComponent
                    }
                }
            }

            Component {
                id: mainMenuComponent
                 Item {
                    anchors.fill: parent

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Text1")
                    }

                    /* ComboBox {
                        id: workspaceComboBox
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 12
                        anchors.bottomMargin: 12

                        model: [qsTr("WorkspaceCreateNew"), qsTr("WorkSpaceOpen")]
                        currentIndex: -1
                        displayText: currentIndex === -1 ? qsTr("WorkSpaceSelect") : currentText

                        onActivated: function(index) {
                            root.handleSelectionChange(index)
                            currentIndex = -1
                        }
                    } */
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
            id: openWorkspaceItem

            property var settingsRef: settings
            property var allWorkspaces: []
            property var workspaceModel: allWorkspaces
            property string pendingDeleteName: ""

            function refreshModel() {
                allWorkspaces = openWorkspaceItem.settingsRef.getWorkspaces();
                applyFilter();
            }

            function applyFilter() {
                const q = searchField.text ? searchField.text.toLowerCase() : "";
                if (!q) {
                    workspaceModel = allWorkspaces;
                } else {
                    // allWorkspaces is expected to be an array of names
                    workspaceModel = allWorkspaces.filter(function(name) { return String(name).toLowerCase().indexOf(q) !== -1; });
                }
            }

            ColumnLayout {  
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                Component.onCompleted: openWorkspaceItem.refreshModel()

                // Header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Open Workspace")
                        font.pixelSize: 22
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Refresh")
                        onClicked: openWorkspaceItem.refreshModel()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search workspaces...")
                        onTextChanged: openWorkspaceItem.applyFilter()
                        selectByMouse: true
                    }
                    Button {
                        text: qsTr("Open Selected")
                        enabled: workspaceListView.currentIndex >= 0 && workspaceListView.count > 0
                        onClicked: {
                            const idx = workspaceListView.currentIndex;
                            if (idx < 0) return;
                            const name = openWorkspaceItem.workspaceModel[idx];
                            const workspace = openWorkspaceItem.settingsRef.getWorkspaceWithName(name);
                            stackView.push(mainUserPageComponent, { workspacePath: workspace[1] });
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 0

                    ScrollView {
                        anchors.fill: parent
                        clip: true

                        ListView {
                            id: workspaceListView
                            anchors.fill: parent
                            model: openWorkspaceItem.workspaceModel
                            spacing: 0
                            currentIndex: -1
                            boundsBehavior: Flickable.StopAtBounds
                            highlightFollowsCurrentItem: false
                            delegate: Item {
                                id: rowDelegate
                                required property var modelData
                                width: ListView.view.width
                                implicitHeight: card.implicitHeight

                                Rectangle {
                                    anchors.fill: parent
                                    color: ListView.isCurrentItem ? Qt.rgba(0,0,0,0.08) : "transparent"
                                }

                                ColumnLayout {
                                    id: card
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: 8
                                    spacing: 4
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        Label {
                                            text: rowDelegate.modelData
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Button {
                                            text: qsTr("Open")
                                            onClicked: {
                                                const ws = openWorkspaceItem.settingsRef.getWorkspaceWithName(rowDelegate.modelData);
                                                stackView.push(mainUserPageComponent, { workspacePath: ws[1] });
                                            }
                                        }
                                        Button {
                                            text: qsTr("Delete")
                                            onClicked: {
                                                openWorkspaceItem.pendingDeleteName = rowDelegate.modelData;
                                                deleteConfirmDialog.open();
                                            }
                                        }
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: {
                                            var ws = openWorkspaceItem.settingsRef.getWorkspaceWithName(rowDelegate.modelData);
                                            return ws && ws.length > 1 ? ws[1] : "";
                                        }
                                        color: Material.hintTextColor
                                        elide: Text.ElideMiddle
                                    }
                                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Qt.rgba(0,0,0,0.1) }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    z: -1
                                    onClicked: function(mouse) {
                                        var pt = workspaceListView.mapFromItem(rowDelegate, mouse.x, mouse.y);
                                        workspaceListView.currentIndex = workspaceListView.indexAt(pt.x, pt.y);
                                    }
                                    onDoubleClicked: {
                                        const ws = openWorkspaceItem.settingsRef.getWorkspaceWithName(rowDelegate.modelData);
                                        stackView.push(mainUserPageComponent, { workspacePath: ws[1] });
                                    }
                                }
                            }

                            Loader {
                                anchors.centerIn: parent
                                active: workspaceListView.count === 0
                                sourceComponent: Column {
                                    spacing: 8
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    Label { text: qsTr("No workspaces to show") }
                                    Label {
                                        text: searchField.text && searchField.text.length > 0
                                              ? qsTr("Try a different search or clear the filter")
                                              : qsTr("Use 'Open from Folder' to import, or create one from the main menu")
                                        color: Material.hintTextColor
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Button {
                        text: qsTr("Open from Folder")
                        onClicked: folderDialog.open()
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Back")
                        onClicked: stackView.pop()
                    }
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

            Dialog {
                id: deleteConfirmDialog
                title: qsTr("Delete workspace?")
                modal: true
                standardButtons: Dialog.Ok | Dialog.Cancel
                contentItem: Column {
                    width: Math.min(440, root.width - 64)
                    spacing: 8
                    Label {
                        text: qsTr("Are you sure you want to delete '%1'? This cannot be undone.")
                              .arg(openWorkspaceItem.pendingDeleteName)
                        wrapMode: Text.WordWrap
                    }
                }
                onAccepted: {
                    const result = openWorkspaceItem.settingsRef.deleteWorkspace(openWorkspaceItem.pendingDeleteName);
                    if (result === 0) {
                        openWorkspaceItem.refreshModel();
                    } else {
                        errorLabel.text = qsTr("Failed to delete workspace");
                        errorDialog.open();
                    }
                    openWorkspaceItem.pendingDeleteName = "";
                }
                onRejected: openWorkspaceItem.pendingDeleteName = ""
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

        contentItem: Label {
            text: qsTr("MessageDialogText")
            horizontalAlignment: Text.AlignHCenter
        }

        onAccepted: {
            root.forceClose = true;
            root.close();
        }
    }

    Dialog {
        id: errorDialog
        title: qsTr("Error")
        modal: true
        standardButtons: Dialog.Ok
        anchors.centerIn: parent

        contentItem: Label {
            id: errorLabel
            text: qsTr("An error occurred")
            horizontalAlignment: Text.AlignHCenter
        }

        onAccepted: {
            // Handle error dialog acceptance if needed
        }
    }
}