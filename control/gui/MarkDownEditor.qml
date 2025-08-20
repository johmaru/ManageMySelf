import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {
    id: markdownEditorPage

    signal backRequested()
    signal requestLoadMarkdownFile(string path)
    signal requestWriteMarkdownFile(string path, string content)

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
                        text: qsTr("Save")
                        onTriggered: {
                            if (markdownContentPath !== "") {
                                markdownEditorPage.requestWriteMarkdownFile(markdownContentPath, mainEditor.text);
                            }
                        }
                    }
                    MenuItem {
                        text: qsTr("Back")
                        onTriggered: markdownEditorPage.backRequested()
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
            requestLoadMarkdownFile(markdownContentPath);
        }
    }

    Column {
        anchors.fill: parent

        RowLayout {
            width: parent.width

            Button {
                text: "Bold"
                onClicked: mainEditor.insertMarkdown("**", "**")
            }

            Button {
                text: "Italic"
                onClicked: mainEditor.insertMarkdown("*", "*")
            }

            Button {
                text: "Code"
                onClicked: mainEditor.insertMarkdown("`", "`")
            }
        }

        ScrollView {
            width: parent.width
            height: parent.height - 50

            TextArea {
                id: mainEditor
                text: rawMarkdownContent
                wrapMode: TextArea.Wrap
                selectByMouse: true
                font.family: "monospace"

                function insertMarkdown(before, after) {
                    var start = selectionStart
                    var end = selectionEnd
                    var selectedText = text.substring(start, end);

                    var newText = text.substring(0, start) +
                        before + selectedText + after +
                        text.substring(end);

                    text = newText;
                    cursorPosition = start + before.length + selectedText.length + after.length;
                }
            }
        }
    }
}