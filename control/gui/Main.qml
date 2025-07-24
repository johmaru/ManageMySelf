import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true

    width: settings.windowWidth
    height: settings.windowHeight
    title: "Manage My Self Main"

    Label {
        anchors.centerIn: parent
        text: "Hello From QML"
    }
}