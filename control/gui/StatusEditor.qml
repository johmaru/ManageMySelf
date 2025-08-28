import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {
    id: statusEditorPage

    signal backRequested()

    property int year: 0
    property int month: 0
    property int day: 0
    property string statusJson: ""
    property string workspacePath: ""


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
}
