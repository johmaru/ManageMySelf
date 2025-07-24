import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow {
    id: root
    visible: true

    width: settings.windowWidth
    height: settings.windowHeight

    Material.theme: settings.theme === "light" ? Material.Light : Material.Dark
    title: "Manage My Self Main"

    Label {
        anchors.centerIn: parent
        text: "Hello From QML"
    }

    ComboBox {
        id: workspaceComboBox

        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 12
        anchors.bottomMargin: 12

        model: ["新規作成","開く"]
    }
}