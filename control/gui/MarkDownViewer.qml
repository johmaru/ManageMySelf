import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {
    id: markdownViewerPage

    signal backRequested
    signal requestLoadMarkdownFile(string path)

    property string markdownContentPath: ""
    property string themeSettings: "light"
    property string rawMarkdownContent: ""

    property Component headerComponent: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("Tool")
                onClicked: toolMenu.open()

                Menu {
                    id: toolMenu

                    MenuItem {
                        text: qsTr("Refresh")
                        onTriggered: {
                            if (markdownContentPath !== "") {
                                markdownViewerPage.requestLoadMarkdownFile(markdownContentPath);
                            }
                        }
                    }
                    MenuItem {
                        text: qsTr("Back")
                        onTriggered: markdownViewerPage.backRequested()
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

        if (markdownContentPath !== "") {
            markdownViewerPage.requestLoadMarkdownFile(markdownContentPath);
        }
    }

    function updateMarkdownContent(content) {
        rawMarkdownContent = content;
        markdownView.text = content;
    }

    TextArea {
        id: markdownView
        anchors.fill: parent
        anchors.margins: 20
        readOnly: true
        selectByMouse: true
        wrapMode: TextEdit.Wrap
        textFormat: TextEdit.MarkdownText
        background: null
        color: Material.primaryTextColor
        font.pixelSize: 14
    }
}
