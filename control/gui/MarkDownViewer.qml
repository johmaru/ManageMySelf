import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material


Page{
    id: markdownViewerPage

    signal backRequested()
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
                        text: qsTr("Back")
                        onTriggered: markdownViewerPage.backRequested()
                    }
                    MenuItem {
                        text: qsTr("Refresh")
                        onTriggered: {
                            if (markdownContentPath !== "") {
                                markdownViewerPage.requestLoadMarkdownFile(markdownContentPath)
                            }
                        }
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
            markdownViewerPage.requestLoadMarkdownFile(markdownContentPath)
        }
    }

    function updateMarkdownContent(content) {
        rawMarkdownContent = content;
        markdownRenderer.renderContent();
    }

    ScrollView{
        anchors.fill: parent
        anchors.margins: 20

        Column {
            id: markdownRenderer
            width: parent.width
            spacing: 10

            function renderContent() {
                for (var i = children.length - 1; i >= 0; i--) {
                    children[i].destroy();
                }

                if (!rawMarkdownContent) return;

                var lines = rawMarkdownContent.split('\n');
                var currentParagraph = "";

                for (var i = 0; i < lines.length; i++) {
                    var line = lines[i];
                    
                    if (line.match(/^#{1,3}\s/)) {
                        if (currentParagraph) {
                            createParagraph(currentParagraph);
                            currentParagraph = "";
                        }
                        createHeader(line);
                    }
                    else if (line.match(/^```/)) {
                        if (currentParagraph) {
                            createParagraph(currentParagraph);
                            currentParagraph = "";
                        }
                    }
                    else if (line.match(/^[-*+]\s/)) {
                        if (currentParagraph) {
                            createParagraph(currentParagraph);
                            currentParagraph = "";
                        }
                        createListItem(line);
                    }
                    else if (line.trim() === "") {
                        if (currentParagraph) {
                            createParagraph(currentParagraph);
                            currentParagraph = "";
                        }
                    }
                    else {
                        currentParagraph += (currentParagraph ? " " : "") + line;
                    }
                }

                if (currentParagraph) {
                    createParagraph(currentParagraph);
                }
            }

            function createHeader(line) {
                var level = (line.match(/^#+/) || [""])[0].length;
                var text = line.replace(/^#+\s*/, "");
                
                Qt.createQmlObject('
                    import QtQuick 2.15
                    import QtQuick.Controls.Material 2.15
                    Text {
                        width: ' + markdownRenderer.width + '
                        text: "' + text.replace(/"/g, '\\"') + '"
                        font.pixelSize: ' + (level === 1 ? 24 : level === 2 ? 20 : 16) + '
                        font.bold: true
                        color: Material.accent
                        wrapMode: Text.Wrap
                    }
                ', markdownRenderer);
            }

            function createParagraph(text) {
                var processedText = processInlineMarkdown(text);
                
                Qt.createQmlObject('
                    import QtQuick 2.15
                    import QtQuick.Controls.Material 2.15
                    Text {
                        width: ' + markdownRenderer.width + '
                        text: "' + processedText.replace(/"/g, '\\"') + '"
                        wrapMode: Text.Wrap
                        textFormat: Text.RichText
                        color: Material.primaryTextColor
                        font.pixelSize: 14
                    }
                ', markdownRenderer);
            }

            function createListItem(line) {
                var text = line.replace(/^[-*+]\s*/, "");
                var processedText = processInlineMarkdown(text);
                
                Qt.createQmlObject('
                    import QtQuick 2.15
                    import QtQuick.Controls.Material 2.15
                    Row {
                        width: ' + markdownRenderer.width + '
                        Text {
                            text: "• "
                            color: Material.primaryTextColor
                            font.pixelSize: 14
                        }
                        Text {
                            width: parent.width - 20
                            text: "' + processedText.replace(/"/g, '\\"') + '"
                            wrapMode: Text.Wrap
                            textFormat: Text.RichText
                            color: Material.primaryTextColor
                            font.pixelSize: 14
                        }
                    }
                ', markdownRenderer);
            }

            function processInlineMarkdown(text) {
                return text
                    .replace(/\*\*(.*?)\*\*/g, '<b>$1</b>')
                    .replace(/\*(.*?)\*/g, '<i>$1</i>')
                    .replace(/`(.*?)`/g, '<code style="background-color: #f0f0f0; padding: 2px 4px;">$1</code>');
            }
        }
    }
}