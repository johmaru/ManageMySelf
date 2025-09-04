pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material
import ManageMySelf.GUI 1.0

Page {
    id: statusEditorPage

    signal backRequested()
    signal requestEditStatus(int year, int month, int day, string statusJson,string path)

    property int year: 0
    property int month: 0
    property int day: 0
    property string statusJson: ""
    property string workspacePath: ""

    property int mode: 0 // 0: show, 1: edit

    property var status: ({})
    property int mood: 0

    property int sleep_hour: 0
    property int sleep_minute: 0

    property int wake_hour: 0
    property int wake_minute: 0

    function toHourPointMinute(h, m) {
        return Number(h + "." + String(m).padStart(2, '0'))
    }

    function toHourMinuteDoubleValue(HHmm) {
        var n = Number(HHmm) || 0
        var h = Math.floor(n)
        var m = Math.round((n - h) * 100)
        if (m >= 60) { h += Math.floor(m / 60); m = m % 60 }
        if (m < 0) m = 0
        if (m > 59) m = 59
        return { hour: h, minute: m }
    }

    
    function handleActivityChange(index) {
        if (index === 0) {
            if (sidePanel.isSelected) {
                sidePanel.isSelected = false
            } else {
                sidePanel.isSelected = true
            }
        } else if (index === 1) {
            sidePanel.isSelected = false
            var sleepDoubleValue = statusEditorPage.toHourPointMinute(statusEditorPage.sleep_hour, statusEditorPage.sleep_minute);
            statusEditorPage.status.sleep_time = sleepDoubleValue;

            var wakeDoubleValue = statusEditorPage.toHourPointMinute(statusEditorPage.wake_hour, statusEditorPage.wake_minute);
            statusEditorPage.status.wake_up_time = wakeDoubleValue;

            var payload = JSON.stringify({
                                mood: statusEditorPage.mood,
                                free_mood_text: statusEditorPage.status.free_mood_text || "",
                                sleep_time: statusEditorPage.toHourPointMinute(statusEditorPage.sleep_hour, statusEditorPage.sleep_minute),
                                wake_up_time: statusEditorPage.toHourPointMinute(statusEditorPage.wake_hour, statusEditorPage.wake_minute),
                                temperature: Number(statusEditorPage.status.temperature || 0.0)
                            })
            requestEditStatus(statusEditorPage.year, statusEditorPage.month, statusEditorPage.day, payload, statusEditorPage.workspacePath)
        } else if (index === 2) {
            sidePanel.isSelected = true
        }
    }

    function loadStatus() {
        const s = extractStatusFromJson();
        status = s || {
            mood: 0,
            free_mood_text: "",
            sleep_time: 0.0,
            wake_up_time: 0.0,
            temperature: 0.0,
            last_modified: "",
            created_at: ""
        }
        mood = status.mood;

        var hm = statusEditorPage.toHourMinuteDoubleValue(status.sleep_time);
        sleep_hour = hm.hour;
        sleep_minute = hm.minute;

        var wm = statusEditorPage.toHourMinuteDoubleValue(status.wake_up_time);
        wake_hour = wm.hour;
        wake_minute = wm.minute;
    }

    function extractStatusFromJson() {
        try {
            var o = JSON.parse(statusJson);
            function pick(keys, def) {
                for (var i = 0; i < keys.length; i++) {
                    var v = o[keys[i]];
                    if (v !== undefined && v !== null) return v;
                }
                return def;
            }
            var toNum = function(v) { var n = Number(v); return isNaN(n) ? 0 : n; };

            return {
                mood: toNum(pick(['mood'], 0)),
                free_mood_text: pick(['free_mood_text', 'freeMoodText', 'freeTextMood', 'freeText'], ""),
                sleep_time: toNum(pick(['sleep_time', 'sleepTime'], 0.0)),
                wake_up_time: toNum(pick(['wake_up_time', 'wakeUpTime'], 0.0)),
                temperature: Number(pick(['temperature', 'temp'], 0.0)) || 0.0,
                last_modified: pick(['last_modified', 'lastModified', 'updated_at', 'updatedAt'], ""),
                created_at: pick(['created_at', 'createdAt', 'date'], "")
            };
        } catch (e) {
            console.error("Failed to parse status JSON:", e);
            return null;
        }
    }

    function getMoodText(mood) {
        switch(mood) {
            case 0: return qsTr("Very Bad");
            case 1: return qsTr("Bad");
            case 2: return qsTr("Neutral");
            case 3: return qsTr("Good");
            case 4: return qsTr("Very Good");
            case 5: return qsTr("Excellent");
            default: return qsTr("Unknown");
        }
    }

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

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ActivityBar {
            id: activityLoader
            Layout.preferredWidth: 56
            Layout.fillHeight: true
            // onCurrentIndexChanged: statusEditorPage.handleActivityChange(currentIndex)
            onActivated: function(idx) { statusEditorPage.handleActivityChange(idx) }
        }

        // サイドパネル
        Frame {
            id: sidePanel
            Layout.preferredWidth: 280
            Layout.fillHeight: true

            property bool isSelected: false

            visible: isSelected

            StackLayout {
                id: sideStack
                anchors.fill: parent
                currentIndex: activityLoader.currentIndex === 2 ? 1 : 0

                Column {
                    spacing: 8
                    padding: 8
                }

                Column {
                    spacing: 8
                    padding: 8
                    Label { text: qsTr("Settings") }
                    Label { text: qsTr("Currently does not support this feature") }
                }
            }
        }

        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true

            padding: 0
            topPadding: 0
            bottomPadding: 0
            leftPadding: 0
            rightPadding: 0

            Loader {
                id: mainContentLoader
                anchors.fill: parent
                sourceComponent: statusEditorPage.mode === 0 ? showComponent : editComponent
            }
        }
    }

    Component {
        id: showComponent

        ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Status Editor for %1-%2-%3")
                            .arg(statusEditorPage.year)
                            .arg(statusEditorPage.month)
                            .arg(statusEditorPage.day)
                        font.pixelSize: 20
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label { text: qsTr("Mood") }
                    Label { text: getMoodText(statusEditorPage.status.mood) }
                }
            
                TextArea {
                    Layout.fillWidth: true

                    text: statusEditorPage.status.free_mood_text
                    readOnly: true
                }

            }
    }


    Component {
        id: editComponent

        ScrollView {
            anchors.fill: parent
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Flickable {
                anchors.fill: parent
                contentWidth: width
                contentHeight: contentCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: contentCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 30

                    Label {
                        width: parent.width
                        padding: 0
                        topPadding: 0
                        bottomPadding: 0
                        text: qsTr("Status Editor for %1-%2-%3")
                            .arg(statusEditorPage.year)
                            .arg(statusEditorPage.month)
                            .arg(statusEditorPage.day)
                        font.pixelSize: 20
                    }

                    RowLayout {
                        width: parent.width
                        spacing: 8

                        Label { text: qsTr("Mood") }

                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 5; stepSize: 1
                            value: statusEditorPage.mood
                            onValueChanged: statusEditorPage.mood = Math.round(value)
                        }

                        Label { text: getMoodText(statusEditorPage.mood) }
                    }

                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Free Mood Text"); font.pixelSize: 20 }

                    ScrollView {
                            width: Math.min(600, parent.width)
                            height: 240
                            anchors.horizontalCenter: parent.horizontalCenter
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            
                            TextArea {
                                id: freeMoodTextArea
                                wrapMode: TextArea.Wrap
                                padding: 0
                                text: statusEditorPage.status.free_mood_text
                                onTextChanged: statusEditorPage.status.free_mood_text = text
                            }
                    }

                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Sleep Time"); font.pixelSize: 20 }

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        Label { text: qsTr("Hour") }
                        SpinBox {
                            id: sleep_hh
                            from: 0; to: 23; stepSize: 1
                            wrap: true
                            focusPolicy: Qt.StrongFocus
                            Keys.onLeftPressed:  { value = value - stepSize; event.accepted = true }
                            Keys.onRightPressed: { value = value + stepSize; event.accepted = true }
                            Keys.onUpPressed:    { value = value + stepSize; event.accepted = true }
                            Keys.onDownPressed:  { value = value - stepSize; event.accepted = true }

                            value: statusEditorPage.toHourMinuteDoubleValue(statusEditorPage.status.sleep_time).hour || 0

                            onValueChanged: statusEditorPage.sleep_hour = value
                        }
                        Label { text: qsTr("Minute") }
                        Label { text: ":" }
                        SpinBox {
                            id: sleep_mm
                            from: 0; to: 59; stepSize: 1
                            wrap: true
                            focusPolicy: Qt.StrongFocus
                            Keys.onLeftPressed:  { value = value - stepSize; event.accepted = true }
                            Keys.onRightPressed: { value = value + stepSize; event.accepted = true }
                            Keys.onUpPressed:    { value = value + stepSize; event.accepted = true }
                            Keys.onDownPressed:  { value = value - stepSize; event.accepted = true }

                            value: statusEditorPage.toHourMinuteDoubleValue(statusEditorPage.status.sleep_time).minute || 0

                            onValueChanged: statusEditorPage.sleep_minute = value
                        }
                        Label { text: Qt.formatTime(new Date(2000, 0, 1, sleep_hh.value, sleep_mm.value, 0), "hh:mm") }
                    }

                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Wake Up Time"); font.pixelSize: 20 }

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        Label { text: qsTr("Hour") }
                        SpinBox {
                            id: wake_hh
                            from: 0; to: 23; stepSize: 1
                            wrap: true
                            focusPolicy: Qt.StrongFocus
                            Keys.onLeftPressed:  { value = value - stepSize; event.accepted = true }
                            Keys.onRightPressed: { value = value + stepSize; event.accepted = true }
                            Keys.onUpPressed:    { value = value + stepSize; event.accepted = true }
                            Keys.onDownPressed:  { value = value - stepSize; event.accepted = true }

                            value: statusEditorPage.toHourMinuteDoubleValue(statusEditorPage.status.wake_up_time).hour || 0
                            onValueChanged: statusEditorPage.wake_hour = value
                        }
                        Label { text: qsTr("Minute") }
                        Label { text: ":" }
                        SpinBox {
                            id: wake_mm
                            from: 0; to: 59; stepSize: 1
                            wrap: true
                            focusPolicy: Qt.StrongFocus
                            Keys.onLeftPressed:  { value = value - stepSize; event.accepted = true }
                            Keys.onRightPressed: { value = value + stepSize; event.accepted = true }
                            Keys.onUpPressed:    { value = value + stepSize; event.accepted = true }
                            Keys.onDownPressed:  { value = value - stepSize; event.accepted = true }

                            value: statusEditorPage.toHourMinuteDoubleValue(statusEditorPage.status.wake_up_time).minute || 0

                            onValueChanged: statusEditorPage.wake_minute = value
                        }
                        Label { text: Qt.formatTime(new Date(2000, 0, 1, wake_hh.value, wake_mm.value, 0), "hh:mm") }
                    }

                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Temperature"); font.pixelSize: 20 }

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        Label { text: qsTr("Value") }
                        Slider {
                            id: tempSlider
                            from: 30.0; to: 45.0; stepSize: 0.1
                            Layout.preferredWidth: 220
                            value: statusEditorPage.status.temperature || 37.0
                            onValueChanged: {
                                var v = Number(value.toFixed(1))
                                if (statusEditorPage.status.temperature !== v)
                                    statusEditorPage.status.temperature = v
                                if (tempField.text !== v.toFixed(1))
                                    tempField.text = v.toFixed(1)
                            }
                        }
                        TextField {
                            id: tempField
                            width: 80
                            text: (statusEditorPage.status.temperature || 37.0).toFixed(1)
                            validator: DoubleValidator { bottom: 30.0; top: 45.0; decimals: 1 }
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            onEditingFinished: {
                                var v = parseFloat(text)
                                if (isNaN(v)) { text = (statusEditorPage.status.temperature || 37.0).toFixed(1); return }
                                v = Math.max(30.0, Math.min(45.0, Math.round(v * 10) / 10))
                                if (statusEditorPage.status.temperature !== v)
                                    statusEditorPage.status.temperature = v
                                if (tempSlider.value !== v)
                                    tempSlider.value = v
                                text = v.toFixed(1)
                            }
                        }
                        Label { text: qsTr("°C") }
                    }
                }
            }
        }
    }

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active)
            loadStatus()
    }

    onStatusJsonChanged: loadStatus()
}
