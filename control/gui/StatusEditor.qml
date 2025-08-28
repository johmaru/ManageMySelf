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

    
    function handleActivityChange(index) {
        // Handle activity change if needed
        console.log("Activity changed to index: " + index);
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
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ActivityBar {
            id: activityLoader
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            onCurrentIndexChanged: statusEditorPage.handleActivityChange(currentIndex)
        }

       /*  // 中: サイドパネル（選択に応じて切替）
        Frame {
            id: sidePanel
            Layout.preferredWidth: 280
            Layout.fillHeight: true

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
        } */

        // 右: メインエリア（元の内容）
        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true

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

                // 編集用の入力欄（存在しない場合のプレースホルダ）
                TextArea {
                    id: statusInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: qsTr("Write your status…")
                }
            }
        }
    }
}
