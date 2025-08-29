pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material
import ManageMySelf.GUI 1.0

Page {
    id: statusEditorPage

    signal backRequested()
    signal requestEditStatus(int year, int month, int day, string statusJson,string path)

    property int year: 0
    property int month: 0
    property int day: 0
    property string statusJson: ""
    property string workspacePath: ""

    property int mode: 0 // 0: show, 1: edit

    property var status: ({})
    property int mood: 0

    
    function handleActivityChange(index) {
        if (index === 0) {
            if (sidePanel.isSelected) {
                sidePanel.isSelected = false
            } else {
                sidePanel.isSelected = true
            }
        } else if (index === 1) {
            sidePanel.isSelected = false
        } else if (index === 2) {
            sidePanel.isSelected = true
        }
    }

    function loadStatus() {
        const s = extractStatusFromJson();
        status = s || {
            mood: 0,
            free_mood_text: "",
            sleep_time: 0,
            wake_up_time: 0,
            temperature: 0.0,
            last_modified: "",
            created_at: ""
        }
        mood = status.mood;
    }

    function extractStatusFromJson() {
        try {
            var status = JSON.parse(statusJson);
            var toNum = v => {
                var n = Number(v);
                return isNaN(n) ? 0 : n;
            };
            return {
                mood: toNum(status['mood']),
                free_mood_text: status['free_mood_text'] || "",
                sleep_time: toNum(status['sleep_time']),
                wake_up_time: toNum(status['wake_up_time']),
                temperature: Number(status['temperature']) || 0.0,
                last_modified: status['last_modified'] || "",
                created_at: status['created_at'] || ""
            };
        } catch (e) {
            console.error("Failed to parse status JSON:", e);
            return null;
        }
    }

    function getMoodText(mood) {
        switch(mood) {
            case 0: return qsTr("Very Bad");
            case 1: return qsTr("Bad");
            case 2: return qsTr("Neutral");
            case 3: return qsTr("Good");
            case 4: return qsTr("Very Good");
            case 5: return qsTr("Excellent");
            default: return qsTr("Unknown");
        }
    }

    property string themeSettings: "light"

    property Component headerComponent: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("Tool")
                onClicked: toolMenu.open()

                Menu {
                    id: toolMenu

                    MenuItem {
                        text: qsTr("Back")
                        onTriggered: statusEditorPage.backRequested()
                    }
                }
            }
        }
    }

    Material.theme: themeSettings === "light" ? Material.Light : Material.Dark
    Material.accent: Material.Blue

    Component.onCompleted: {
        var theme = settings.theme || "dark";
        themeSettings = theme;
        loadStatus();
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ActivityBar {
            id: activityLoader
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            onCurrentIndexChanged: statusEditorPage.handleActivityChange(currentIndex)
            onActivated: function(idx) { statusEditorPage.handleActivityChange(idx) }
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
                currentIndex: activityLoader.currentIndex

                Column {
                    spacing: 8
                    padding: 8
                    
                }

                Column {
                    spacing: 8
                    padding: 8
                    Label { text: qsTr("Settings") }
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
                sourceComponent: statusEditorPage.mode === 0 ? showComponent : editComponent
            }
        }
    }

    Component {
        id: showComponent

        ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Status Editor for %1-%2-%3")
                            .arg(statusEditorPage.year)
                            .arg(statusEditorPage.month)
                            .arg(statusEditorPage.day)
                        font.pixelSize: 20
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    }
                }

            RowLayout {
                Layout.fillWidth: true

                Label { text: qsTr("Mood") }
                Label { text: getMoodText(statusEditorPage.status.mood) }
            }

            }
    }


    Component {
        id: editComponent

        ScrollView {
            anchors.fill: parent
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Flickable {
                anchors.fill: parent
                contentWidth: width
                contentHeight: contentCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: contentCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 30

                    Label {
                        width: parent.width
                        padding: 0
                        topPadding: 0
                        bottomPadding: 0
                        text: qsTr("Status Editor for %1-%2-%3")
                            .arg(statusEditorPage.year)
                            .arg(statusEditorPage.month)
                            .arg(statusEditorPage.day)
                        font.pixelSize: 20
                    }

                    RowLayout {
                        width: parent.width
                        spacing: 8

                        Label { text: qsTr("Mood") }

                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 5; stepSize: 1
                            value: statusEditorPage.mood
                            onValueChanged: statusEditorPage.mood = Math.round(value)
                        }

                        Label { text: getMoodText(statusEditorPage.mood) }
                    }
                }
            }
        }
    }

    onMoodChanged: statusEditorPage.status =
        Object.assign({}, statusEditorPage.status, { mood: statusEditorPage.mood })
}
