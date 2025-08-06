import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {
    id: mainUserPage

    signal showError(string message)
    signal requestGetUserName(string path)
    signal backRequested()
    property string workspacePath: ""
    property string userName: ""

    property Component headerComponent: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "‹ " + qsTr("Back")
                onClicked: mainUserPage.backRequested()
            }
        }
    }

    background: Rectangle {
        color: Material.background
    }

    Component.onCompleted: {
        mainUserPage.requestGetSettings(mainUserPage.workspacePath);
    }

    function requestGetSettings(path) {
        mainUserPage.requestGetUserName(path);
    }

    Label {
        id: welcomeLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        font.pixelSize: 18
        text: qsTr("Welcome") + " " + mainUserPage.userName
    }
}