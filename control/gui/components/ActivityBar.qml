pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Pane {
    id: root
    enum Scene { Main, MainUserPage, StatusEditor }
    property int scene: ActivityBar.Scene.StatusEditor
    property int mode: 0 // 0=normal, 1=edit
    property string currentKey: "toggle"
    signal activated(string key)

    Loader {
        anchors.fill: parent
        sourceComponent: root.scene === ActivityBar.Scene.Main         ? null
                        : root.scene === ActivityBar.Scene.MainUserPage ? null
                        : root.scene === ActivityBar.Scene.StatusEditor ? statusEditorCmp
                        : null
    }

    Component {
        id: statusEditorCmp

        ColumnLayout {
        anchors.fill: parent
        spacing: 4

        ButtonGroup { id: grp; exclusive: true }

        Repeater {
            model: [
                { key: "toggle",  name: qsTr("ToggleView"),  iconSource: "qrc:/icons/toggle-column-svgrepo-com.svg", display: "iconOnly" },
                { key: "home",    name: qsTr("Home"),        iconName: "go-home",                                 display: "textUnder" },
                { key: "save",    name: qsTr("Save"),        iconName: "content-save",                            display: "textUnder" },
                { key: "settings",name: qsTr("Settings"),    iconName: "settings",                                display: "textUnder" }
            ]

            delegate: ToolButton {
                required property var modelData
                property string key: modelData.key

                visible: modelData.key !== "save" || root.mode === 1

                checkable: true
                checked: key === root.currentKey
                ButtonGroup.group: grp

                display: modelData.display === "iconOnly" ? AbstractButton.IconOnly
                                                          : AbstractButton.TextUnderIcon
                text: modelData.name

                icon.source: modelData.iconSource || ""
                icon.name: modelData.iconName || ""
                icon.color: Material.foreground
                icon.width: 20
                icon.height: 20

                onClicked: {
                    if (key !== root.currentKey) root.currentKey = key
                    root.activated(key)
                }

                ToolTip.visible: modelData.display === "iconOnly" && hovered
                ToolTip.text: modelData.name
                ToolTip.delay: 500
            }
        }

        Item { Layout.fillHeight: true }
    }
    }

    onModeChanged: if (mode !== 1 && currentKey === "save") currentKey = "home"
}