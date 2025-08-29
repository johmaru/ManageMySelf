pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Pane {
    id: root

    property int currentIndex: 0
    signal activated(int index)
    Layout.preferredWidth: 56
    Layout.fillHeight: true
    padding: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        ButtonGroup { id: grp; exclusive: true }

        Repeater {
            model: [
                { name: qsTr("ToggleView"), source: "qrc:/icons/toggle-column-svgrepo-com.svg" },
                { name: qsTr("Save"), icon: "content-save" },
                { name: qsTr("Settings"), icon: "settings" }
            ]
            delegate: ToolButton {
                required property int index
                required property var modelData

                checkable: true
                checked: index === root.currentIndex
                ButtonGroup.group: grp

                display: (index === 0) ? AbstractButton.IconOnly : AbstractButton.TextUnderIcon
                text: modelData.name

                icon.source: index === 0 ? modelData.source : ""
                icon.color: Material.foreground
                icon.width: 20
                icon.height: 20

                onClicked: {
                    if (index !== root.currentIndex) root.currentIndex = index
                    root.activated(index)
                }

                ToolTip.visible: (index === 0) && hovered
                ToolTip.text: modelData.name
                ToolTip.delay: 500
            }
        }

        Item { Layout.fillHeight: true }
    }
}