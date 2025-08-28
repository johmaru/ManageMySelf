pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Pane {
    id: root

    property int currentIndex: 0
    Layout.preferredWidth: 56
    Layout.fillHeight: true
    padding: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        ButtonGroup { id: grp; exclusive: true }

        Repeater {
            model: [
                { name: qsTr("Save"), icon: "content-save" },
                { name: qsTr("Settings"), icon: "settings" }
            ]
            delegate: ToolButton {
                required property int index
                required property var modelData

                checkable: true
                checked: index === root.currentIndex
                ButtonGroup.group: grp
                display: AbstractButton.TextUnderIcon
                text: modelData.name

                icon.width: 20
                icon.height: 20
                Layout.alignment: Qt.AlignHCenter
                onClicked: root.currentIndex = index
            }
        }

        Item { Layout.fillHeight: true }
    }
}