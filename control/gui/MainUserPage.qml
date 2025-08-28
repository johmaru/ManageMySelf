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
    signal requestCreateDiary(int year, int month,int day,string title, string path)
    signal requestCreateStatus(string path)
    signal requestMonthUserDiarySqlData(int year, int month, string path)
    signal requestMonthUserStatusData(int year, int month, string path)
    signal updateMonthGridData(string jsonString)
    signal requestNavigateMarkdownEditor(string contentPath)
    signal requestNavigateMarkdownViewer(string contentPath)

    onUpdateMonthGridData: function(jsonString) {
        if (monthGrid) {
            monthGrid.updateMonthData(jsonString);
        } else {
            console.error("MonthGrid not found");
        }
    }

    property string workspacePath: ""
    property string userName: ""
    property string themeSettings: "light"

    property int currentYear: new Date().getFullYear()
    property int currentMonth: new Date().getMonth() + 1

    property string _pendingDiaryJson: ""
    property string _pendingStatusJson: ""
    property bool _diaryArrived: false
    property bool _statusArrived: false

    property Component headerComponent: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("File")
            
                onClicked: fileMenu.open()

                Menu {
                    id: fileMenu
                    MenuItem {
                        text: qsTr("Create Diary")
                        onTriggered: {
                            var today = todayYYmmdd();
                            var selectedDate = mainUserPage.currentYear + "-" + 
                                               ("0" + mainUserPage.currentMonth).slice(-2) + "-" + 
                                               ("0" + monthGrid.currentDay).slice(-2);

                            if (selectedDate === today) {
                                createDiaryDialog.open()
                            } else {
                                dateMissMatchDialog.open()
                            }
                        }
                    }

                    MenuItem {
                        text: qsTr("Back to Workspaces")
                        onTriggered: mainUserPage.backRequested()
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

        console.log("Today's date:", new Date())
        console.log("JavaScript getMonth():", new Date().getMonth())
        console.log("Current month property:", currentMonth)
        console.log("MonthGrid month:", monthGrid.month)
        mainUserPage.requestGetSettings(mainUserPage.workspacePath);
    }

    function todayYYmmdd() {
        var today = new Date();
        
        return today.getFullYear() + "-" + 
               ("0" + (today.getMonth() + 1)).slice(-2) + "-" + 
               ("0" + today.getDate()).slice(-2);
    }

    function requestGetSettings(path) {
        mainUserPage.requestGetUserName(path);
    }

    function reloadMonthData() {
        if (monthGrid) {
            monthGrid.loadMonthData();
        }
    }

    function handleDiaryData(jsonString) {
        _pendingDiaryJson = jsonString
        _diaryArrived = true
        _tryComposeAndUpdate()
    }

    function handleStatusData(jsonString) {
        _pendingStatusJson = jsonString
        _statusArrived = true
        _tryComposeAndUpdate()
    }

    function _tryComposeAndUpdate() {
        if (!(_diaryArrived && _statusArrived))
            return

        var merged = _mergeDiaryAndStatus(_pendingDiaryJson, _pendingStatusJson)
        updateMonthGridData(JSON.stringify(merged))

        _pendingDiaryJson = ""
        _pendingStatusJson = ""
        _diaryArrived = false
        _statusArrived = false
    }

    function _normalizeDiaryArray(obj) {
        if (!obj) return []
        if (Array.isArray(obj)) return obj
        if (obj.diaries && Array.isArray(obj.diaries)) return obj.diaries
        return []
    }

    function _mergeDiaryAndStatus(diaryJson, statusJson) {
        var diaryObj, statusObj
        try { diaryObj = JSON.parse(diaryJson) } catch (e) { diaryObj = {} }
        try { statusObj = JSON.parse(statusJson) } catch (e) { statusObj = {} }

        var diaries = _normalizeDiaryArray(diaryObj)

        var statuses = []
        if (Array.isArray(statusObj)) statuses = statusObj
        else if (Array.isArray(statusObj.statuses)) statuses = statusObj.statuses
        else if (Array.isArray(statusObj.status)) statuses = statusObj.status

        function dateKey(v) {
            if (!v) return ""
            var s = String(v)
            // 'YYYY-MM-DD HH:mm:ss' or ISO 'YYYY-MM-DDTHH:mm:ss'
            var d = s.indexOf('T') >= 0 ? s.split('T')[0] : s.split(' ')[0]
            return d || ""
        }

        var statusByDate = {}
        for (var i = 0; i < statuses.length; i++) {
            var st = statuses[i]
            var key = dateKey(st.createdAt || st.date || st.day)
            if (key) statusByDate[key] = st
        }

        for (var j = 0; j < diaries.length; j++) {
            var d = diaries[j]
            var key = dateKey(d.createdAt)
            var st = key ? statusByDate[key] : null
            if (st) {
                d.status = st
                d.hasStatus = true
            } else {
                d.hasStatus = false
            }
        }

        return { diaries: diaries, statusDates: Object.keys(statusByDate), statusByDate: statusByDate}
    }

    Dialog {
        id: createDiaryDialog
        title: qsTr("Create Diary")
        standardButtons: Dialog.Ok | Dialog.Cancel
    
        contentItem: Column {
            spacing: 10
            width: 300
            
            Label {
                text: qsTr("Title:")
            }
            
            TextField {
                id: diaryTitleField
                width: parent.width
                placeholderText: qsTr("Enter diary title")
            }
        }
        
        onAccepted: {
            var title = diaryTitleField.text.trim()
            
            if (title === "") {
                mainUserPage.showError(qsTr("Title cannot be empty"))
                return
            }

            mainUserPage.requestCreateDiary(monthGrid.year, monthGrid.month + 1, monthGrid.currentDay, title, mainUserPage.workspacePath)

            Qt.callLater(function() {
                monthGrid.loadMonthData()
            })
            
            diaryTitleField.text = ""
        }
        
        onRejected: {
            diaryTitleField.text = ""
        }
    }

    Dialog {
        id: dateMissMatchDialog
        title: qsTr("Date Mismatch")
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: Column {
            spacing: 10
            width: 300

            Label {
                text: qsTr("The selected date does not match the diary entry date. Are you sure you want to continue?")
            }
        }

        onAccepted: {
                createDiaryDialog.open()
            }
    }

    Dialog {
        id: datePickerDialog
        title: qsTr("Date Picker")
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        contentItem: Column {
            spacing: 10
            width: 250
            
            Label { text: "年:" }
            SpinBox {
                id: yearSpinBox
                from: 1900
                to: 2100
                value: mainUserPage.currentYear
                width: parent.width
                height: 30
            }
            
            Label { text: "月:" }
            ComboBox {
                id: monthComboBox
                width: parent.width
                model: ["1月", "2月", "3月", "4月", "5月", "6月", 
                       "7月", "8月", "9月", "10月", "11月", "12月"]
                currentIndex: mainUserPage.currentMonth - 1
            }
        }
        
        onAccepted: {
            mainUserPage.currentYear = yearSpinBox.value
            mainUserPage.currentMonth = monthComboBox.currentIndex + 1
        }
    }

    Label {
        id: welcomeLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        font.pixelSize: 18
        text: qsTr("Welcome") + " " + mainUserPage.userName
    }


    ColumnLayout {
        anchors.topMargin: 20
        anchors.top: welcomeLabel.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10

        ToolButton {
        id: datePickerButton
        text: "日付選択"
        onClicked: datePickerDialog.open()
    }

    Menu {
        id: dayContextMenu

        property int selectedDay: 0
        property string selectedMDContentPath: ""

        Menu {
            id: createItemMenu
            title: qsTr("Create Item")

            property bool hasDiaryItems: dayContextMenu.selectedMDContentPath == ""
            property bool hasStatus: (monthGrid.updateTrigger, !monthGrid.hasStatusForDay(dayContextMenu.selectedDay))

            MenuItem {
                text: qsTr("Create Diary")
                visible: createItemMenu.hasDiaryItems
                enabled: createItemMenu.hasDiaryItems
                onTriggered: {
                    var today = todayYYmmdd();
                    var selectedDate = mainUserPage.currentYear + "-" + 
                        ("0" + mainUserPage.currentMonth).slice(-2) + "-" + 
                        ("0" + monthGrid.currentDay).slice(-2);

                    if (selectedDate === today) {
                        createDiaryDialog.open()
                    } else {
                        dateMissMatchDialog.open()
                    }
                }
            }

            MenuItem {
                text: qsTr("Create Status")
                visible: createItemMenu.hasStatus
                enabled: createItemMenu.hasStatus
                onTriggered: {
                    var today = todayYYmmdd()
                    var selectedDate = mainUserPage.currentYear + "-" +
                                       ("0" + mainUserPage.currentMonth).slice(-2) + "-" +
                                       ("0" + dayContextMenu.selectedDay).slice(-2)

                    if (selectedDate === today) {
                        mainUserPage.requestCreateStatus(mainUserPage.workspacePath)
                    } else {
                        console.warn("Cannot create status for past days:", dayContextMenu.selectedDay)
                    }
                }
            }
        }

        Menu {
            id: editItemMenu
            title: qsTr("Edit Item")

            property bool hasDiaryItems: dayContextMenu.selectedMDContentPath !== ""

            MenuItem {
                text: qsTr("Edit Diary")
                visible: editItemMenu.hasDiaryItems
                enabled: editItemMenu.hasDiaryItems
                onTriggered: {
                    if (dayContextMenu.selectedMDContentPath !== "") {
                        mainUserPage.requestNavigateMarkdownEditor(dayContextMenu.selectedMDContentPath);
                    } else {
                        console.warn("No content path selected for day:", dayContextMenu.selectedDay);
                    }
                }
            }

            MenuItem {
                text: qsTr("Item has not been available")
                visible: !editItemMenu.hasDiaryItems
                enabled: false
            }
        }

        Menu {
        id: showItemMenu
        title: qsTr("Show Item")
        
        property bool hasDiaryItems: dayContextMenu.selectedMDContentPath !== ""
        
        MenuItem {
            text: qsTr("Show Diary")
            visible: showItemMenu.hasDiaryItems
            enabled: showItemMenu.hasDiaryItems
            onTriggered: {
                if (dayContextMenu.selectedMDContentPath !== "") {
                    mainUserPage.requestNavigateMarkdownViewer(dayContextMenu.selectedMDContentPath);
                } else {
                    console.warn("No content path selected for day:", dayContextMenu.selectedDay);
                }
                console.log("Content Path:", dayContextMenu.selectedMDContentPath);
            }
        }
        
        MenuItem {
            text: qsTr("No item has been available")
            visible: !showItemMenu.hasDiaryItems
            enabled: false
        }
    }
    
    }

    GridView {
        id: monthGrid
        width: 280
        height: 240
        cellWidth: 40
        cellHeight: 40
        
        property int month: mainUserPage.currentMonth - 1
        property int year: mainUserPage.currentYear
        property int currentDay: new Date().getDate()
        property var monthData: ({})
        property int updateTrigger: 0
        
        model: 42  // 6週間分
        
        delegate: Rectangle {
            width: 40
            height: 40

            readonly property var  dayDiaries: (monthGrid.updateTrigger, monthGrid.monthData[day] || [])
            readonly property bool hasDiary:   (monthGrid.updateTrigger, monthGrid.hasDiaryForDay(day))
            readonly property bool hasStatus:  (monthGrid.updateTrigger, monthGrid.hasStatusForDay(day))
            readonly property bool hasAny: hasDiary || hasStatus
            
            property int day: {
                var firstDay = new Date(monthGrid.year, monthGrid.month, 1).getDay()
                var dayNumber = index - firstDay + 1
                return (dayNumber > 0 && dayNumber <= new Date(monthGrid.year, monthGrid.month + 1, 0).getDate()) ? dayNumber : 0
            }
            
            readonly property bool today: {
                var now = new Date()
                return day > 0 && 
                       now.getDate() === day && 
                       now.getMonth() === monthGrid.month && 
                       now.getFullYear() === monthGrid.year
            }
            
            color: {
                if (today) return Material.accent
                if (hasAny) return Material.color(Material.LightBlue)
                return Material.backgroundColor
            }  
             border.color: Material.frameColor
            border.width: day > 0 ? 1 : 0
            
            visible: day > 0
            
            Label {
                anchors.centerIn: parent
                text: parent.day
                color: parent.today ? Material.primaryHighlightedTextColor : Material.primaryTextColor
            }
            
            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: Material.color(Material.Blue)
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 2
                visible: parent.hasDiary
            }

            Rectangle {
                width: 6
                height: 6
                radius: 3
                color: Material.color(Material.Red)
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 2
                visible: parent.hasStatus
            }
            
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(mouse) {
                    if (parent.day === 0) return
                    
                    monthGrid.currentDay = parent.day

                    switch (mouse.button) {
                        case Qt.LeftButton:
                            var diaries = parent.dayDiaries
                            console.log("Date:", parent.day, "Diaries:", diaries.length)
                            if (diaries.length > 0) {
                                console.log("Diary titles:", diaries.map(d => d.title))
                            }
                            var status = parent.dayDiaries.length > 0 ? parent.dayDiaries[0].status : null
                            var mood = monthGrid._extractStatusFields(status).mood
                            console.log("Mood:", mood)
                            break;
                        case Qt.RightButton:
                            dayContextMenu.selectedDay = parent.day
                            dayContextMenu.selectedMDContentPath = parent.dayDiaries.length > 0 ? parent.dayDiaries[0].contentPath : ""
                            dayContextMenu.open()
                            break;
                    }
                }
            }
        }

        function hasStatusForDay(day) {
            if (day <= 0) return false
            var items = monthGrid.monthData[day] || []
            for (var i = 0; i < items.length; i++) {
                if (items[i] && (items[i].hasStatus === true || items[i].status)) return true
            }
            return false
        }

        function hasDiaryForDay(day) {
            if (day <= 0) return false
            var items = monthGrid.monthData[day] || []
            for (var i = 0; i < items.length; i++) {
                var it = items[i]
                if (it && it.contentPath) return true
            }
            return false
        }

        function hasAnyForDay(day) {
            return monthGrid.hasDiaryForDay(day) || monthGrid.hasStatusForDay(day)
        }

        function _extractStatusFields(statusObj) {
            if (!statusObj) return { mood: null, freeTextMood: null }
            var mood = statusObj.mood !== undefined ? statusObj.mood : null
            var freeText = statusObj.free_mood_text !== undefined ? statusObj.free_mood_text
                         : (statusObj.freeMoodText !== undefined ? statusObj.freeMoodText : null)
            return { mood: mood, freeTextMood: freeText }
        }

        function hasMoodForDay(day) {
            if (day <= 0) return false
            var items = monthGrid.monthData[day] || []
            for (var i = 0; i < items.length; i++) {
                var s = items[i] ? items[i].status : null
                var f = _extractStatusFields(s)
                if (f.mood !== null && f.mood !== "") return true
            }
            return false
        }

        function getMoodForDay(day) {
            if (day <= 0) return null
            var items = monthGrid.monthData[day] || []
            for (var i = 0; i < items.length; i++) {
                var s = items[i] ? items[i].status : null
                var f = _extractStatusFields(s)
                if (f.mood !== null && f.mood !== "") return f.mood
            }
            return null
        }
        
        function loadMonthData() {
            mainUserPage.requestMonthUserDiarySqlData(mainUserPage.currentYear, mainUserPage.currentMonth, mainUserPage.workspacePath)
            mainUserPage.requestMonthUserStatusData(mainUserPage.currentYear, mainUserPage.currentMonth, mainUserPage.workspacePath)
        }

        function updateMonthData(jsonString) {
            try {
                var raw = JSON.parse(jsonString)

                var data = raw.diaries ? raw.diaries : raw
                var statusDates = raw.statusDates || []

                function parseCreatedAtToDate(s) {
                    if (!s) return null
                    var str = String(s)
                    if (str.indexOf('T') >= 0) {
                        var d = new Date(str)
                        return isNaN(d) ? null : d
                    }
                    var parts = str.split(' ')
                    var ymd = parts[0].split('-')
                    if (ymd.length !== 3) return null
                    var y = Number(ymd[0]), m = Number(ymd[1]) - 1, d = Number(ymd[2])
                    if (isNaN(y) || isNaN(m) || isNaN(d)) return null
                    return new Date(y, m, d)
                }

                function parseYmdToYMD(ymdStr) {
                    if (!ymdStr) return null
                    var ymd = ymdStr.split('-')
                    if (ymd.length !== 3) return null
                    var y = Number(ymd[0]), m = Number(ymd[1]) - 1, d = Number(ymd[2])
                    if (isNaN(y) || isNaN(m) || isNaN(d)) return null
                    return { y: y, m: m, d: d }
                }

                monthData = {}

                for (var i = 0; i < data.length; i++) {
                    var diary = data[i]
                    var dateObj = parseCreatedAtToDate(diary.createdAt)
                    if (!dateObj) {
                        console.warn("Diary missing/invalid createdAt:", diary && diary.title)
                        continue
                    }
                    if (dateObj.getFullYear() === mainUserPage.currentYear && dateObj.getMonth() === monthGrid.month) {
                        var day = dateObj.getDate()
                        if (!monthData[day]) monthData[day] = []
                        monthData[day].push(diary)
                    }
                }

                for (var k = 0; k < statusDates.length; k++) {
                    var ymd = parseYmdToYMD(statusDates[k])
                    if (!ymd) continue
                    if (ymd.y === mainUserPage.currentYear && ymd.m === monthGrid.month) {
                        var dayNum = ymd.d
                        var items = monthData[dayNum] || []
                        var hasAnyStatus = false
                        for (var t = 0; t < items.length; t++) {
                            if (items[t] && (items[t].hasStatus === true || items[t].status)) { hasAnyStatus = true; break }
                        }
                        if (!hasAnyStatus) {
                            if (!monthData[dayNum]) monthData[dayNum] = []
                            monthData[dayNum].push({
                                title: "",
                                contentPath: "",
                                createdAt: statusDates[k] + " 00:00:00",
                                status: (raw.statusByDate && raw.statusByDate[statusDates[k]]) || {},
                                hasStatus: true
                            })
                        }
                    }
                }

                console.log("Final monthData:", JSON.stringify(monthData))
                updateTrigger++
            } catch (e) {
                console.error("updateMonthData error:", e)
            }
        }
        
        onMonthChanged: loadMonthData()
        onYearChanged: loadMonthData()
        
        Component.onCompleted: {
            mainUserPage.requestMonthUserDiarySqlData(mainUserPage.currentYear, mainUserPage.currentMonth, mainUserPage.workspacePath);
            loadMonthData();
        }
    }
    }
}