pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material
import QtQml

Page {
    id: mainUserPage

    property string userName: ""
    property string workspacePath: ""
    property int currentYear: new Date().getFullYear()
    property int currentMonth: new Date().getMonth() + 1

    property var themeSettings: undefined

    property string _pendingDiaryJson: ""
    property string _pendingStatusJson: ""
    property bool _diaryArrived: false
    property bool _statusArrived: false

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
    signal requestStatusEditor(int year, int month, int day, string jsonString, string path, int mode)

    property Component headerComponent: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("File")
            
                onClicked: fileMenu.open()

                Menu {
                    id: fileMenu

                    MenuItem {
                        text: qsTr("Back to Workspaces")
                        onTriggered: mainUserPage.backRequested()
                    }
                }
            }
        }
    }

    onUpdateMonthGridData: function(jsonString) {
        if (monthGrid) {
            monthGrid.updateMonthData(jsonString)
        } else {
            console.error("MonthGrid not found")
        }
    }


    Component.onCompleted: {
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

    function getSettings(path) {
        return mainUserPage.requestGetSettings(path)
    }
    function getMonthUserDiarySqlData(year, month, path) {
        mainUserPage.requestMonthUserDiarySqlData(year, month, path)
    }
    function getMonthUserStatusData(year, month, path) {
        mainUserPage.requestMonthUserStatusData(year, month, path)
    }

    function reloadMonthData() {
        if (monthGrid) {
            monthGrid.loadMonthData();
        }
    }

    function handleDiaryData(jsonString) {
        mainUserPage._pendingDiaryJson = jsonString
        mainUserPage._diaryArrived = true
        mainUserPage._tryComposeAndUpdate()
    }

    function handleStatusData(jsonString) {
        mainUserPage._pendingStatusJson = jsonString
        mainUserPage._statusArrived = true
        mainUserPage._tryComposeAndUpdate()
    }

    function _tryComposeAndUpdate() {
        if (!(mainUserPage._diaryArrived && mainUserPage._statusArrived))
            return

        var merged = mainUserPage._mergeDiaryAndStatus(mainUserPage._pendingDiaryJson, mainUserPage._pendingStatusJson)
        mainUserPage.updateMonthGridData(JSON.stringify(merged))

        mainUserPage._pendingDiaryJson = ""
        mainUserPage._pendingStatusJson = ""
        mainUserPage._diaryArrived = false
        mainUserPage._statusArrived = false
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
        width: 320
    
        contentItem: Column {
            spacing: 10
            
            Label {
                text: qsTr("Title:")
            }
            
            TextField {
                id: diaryTitleField
                width: 280
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
    width: 320

        contentItem: Column {
            spacing: 10
            width: 300

            Label {
                text: qsTr("The selected date does not match the diary entry date.\nAre you sure you want to continue?")
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
        width: 300
        
        contentItem: Column {
            spacing: 10
            
            Label { text: "年:" }
            SpinBox {
                id: yearSpinBox
                from: 1900
                to: 2100
                value: mainUserPage.currentYear
                width: 250
                height: 30
            }
            
            Label { text: "月:" }
            ComboBox {
                id: monthComboBox
                width: 250
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

        GridView {
        id: monthGrid
        Layout.preferredWidth: 280
        Layout.preferredHeight: 240
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
            required property int index

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
                            var items = parent.dayDiaries
                            var statusObj = null
                            for (var i = 0; i < items.length; i++) {
                                var s = items[i] ? items[i].status : null
                                if (s && Object.keys(s).length > 0) { statusObj = s; break }
                            }
                            dayContextMenu.selectedStatusJson = statusObj ? JSON.stringify(statusObj) : ""
                            dayContextMenu.popup(parent, mouse.x, mouse.y)
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
            var sleep_time = statusObj.sleep_time !== undefined ? statusObj.sleep_time
                           : (statusObj.sleepTime !== undefined ? statusObj.sleepTime : null)
            var wake_up_time = statusObj.wake_up_time !== undefined ? statusObj.wake_up_time
                             : (statusObj.wakeUpTime !== undefined ? statusObj.wakeUpTime : null)
            var temperature = statusObj.temperature !== undefined ? statusObj.temperature : null
            return { mood: mood, freeTextMood: freeText, sleepTime: sleep_time, wakeUpTime: wake_up_time, temperature: temperature }
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
            mainUserPage.requestMonthUserDiarySqlData(mainUserPage.currentYear, mainUserPage.currentMonth, mainUserPage.workspacePath)
            loadMonthData()
        }
        }
    }

    Component {
        id: cmpCreateDiary
        MenuItem {
            text: qsTr("Create Diary")
            onTriggered: {
                dayContextMenu.close()
                var today = mainUserPage.todayYYmmdd()
                var selectedDate = mainUserPage.currentYear + "-" + ("0" + mainUserPage.currentMonth).slice(-2) + "-" + ("0" + monthGrid.currentDay).slice(-2)
                if (selectedDate === today) {
                    createDiaryDialog.open()
                } else {
                    dateMissMatchDialog.open()
                }
            }
        }
    }

    Component {
        id: cmpCreateStatus
        MenuItem {
            text: qsTr("Create Status")
            onTriggered: {
                dayContextMenu.close()
                var today = mainUserPage.todayYYmmdd()
                var selectedDate = mainUserPage.currentYear + "-" + ("0" + mainUserPage.currentMonth).slice(-2) + "-" + ("0" + monthGrid.currentDay).slice(-2)
                if (selectedDate === today) {
                    mainUserPage.requestCreateStatus(mainUserPage.workspacePath)
                } else {
                    console.warn("Cannot create status for past days:", monthGrid.currentDay)
                }
            }
        }
    }

    Component {
        id: cmpNoCreateDiary
        MenuItem { text: qsTr("Cannot create diary for past days"); enabled: false }
    }

    Component { id: cmpEditDiary
        MenuItem {
            text: qsTr("Edit Diary")
            onTriggered: {
                dayContextMenu.close()
                if (dayContextMenu.selectedMDContentPath !== "")
                    mainUserPage.requestNavigateMarkdownEditor(dayContextMenu.selectedMDContentPath)
            }
        }
    }
    Component { id: cmpEditStatus
        MenuItem {
            text: qsTr("Edit Status")
            onTriggered: {
                dayContextMenu.close()
                mainUserPage.requestStatusEditor(
                    mainUserPage.currentYear,
                    mainUserPage.currentMonth,
                    dayContextMenu.selectedDay,
                    dayContextMenu.selectedStatusJson,
                    mainUserPage.workspacePath,
                    1
                )
            }
        }
    }
    Component { id: cmpNoItem
        MenuItem { text: qsTr("Item has not been available"); enabled: false }
    }

    Component {
        id: cmpShowDiary
        MenuItem {
            text: qsTr("Show Diary")
            onTriggered: {
                dayContextMenu.close()
                if (dayContextMenu.selectedMDContentPath !== "") {
                    mainUserPage.requestNavigateMarkdownViewer(dayContextMenu.selectedMDContentPath)
                } else {
                    console.warn("No content path selected for day:", dayContextMenu.selectedDay)
                }
                console.log("Content Path:", dayContextMenu.selectedMDContentPath)
            }
        }
    }

    Component {
        id: cmpShowStatus
        MenuItem {
            text: qsTr("Show Status")
            onTriggered: {
                dayContextMenu.close()
                mainUserPage.requestStatusEditor(
                    mainUserPage.currentYear,
                    mainUserPage.currentMonth,
                    dayContextMenu.selectedDay,
                    dayContextMenu.selectedStatusJson,
                    mainUserPage.workspacePath,
                    0
                )
            }
        }
    }

    Menu {
        id: dayContextMenu

        property int selectedDay: 0
        property string selectedMDContentPath: ""
        property string selectedStatusJson: ""

        Menu {
            id: createItemMenu
            title: qsTr("Create Item")

            property bool hasDiaryItems: dayContextMenu.selectedMDContentPath == ""
            property bool hasStatus: (monthGrid.updateTrigger, !monthGrid.hasStatusForDay(dayContextMenu.selectedDay))

            property var __dynItems: []
            onAboutToShow: {
                for (var i = 0; i < __dynItems.length; i++) {
                    var obj = __dynItems[i]
                    if (obj) {
                        try { createItemMenu.removeItem(obj) } catch(e) {}
                        try { obj.destroy() } catch(e) {}
                    }
                }
                __dynItems = []

                const showCreateDiary = createItemMenu.hasDiaryItems
                const showCreateStatus = createItemMenu.hasStatus

                if (showCreateDiary) {
                    var d = cmpCreateDiary.createObject(null)
                    createItemMenu.addItem(d)
                    __dynItems.push(d)
                }
                if (showCreateStatus) {
                    var s = cmpCreateStatus.createObject(null)
                    createItemMenu.addItem(s)
                    __dynItems.push(s)
                }
                if (!showCreateDiary && !showCreateStatus) {
                    var n = cmpNoCreateDiary.createObject(null)
                    createItemMenu.addItem(n)
                    __dynItems.push(n)
                }
            }
        }

        Menu {
            id: editItemMenu
            title: qsTr("Edit Item")

            property var __dynItems: []
            onAboutToShow: {
                for (var i = 0; i < __dynItems.length; i++) {
                    var obj = __dynItems[i]
                    if (obj) {
                        try { editItemMenu.removeItem(obj) } catch(e) {}
                        try { obj.destroy() } catch(e) {}
                    }
                }
                __dynItems = []

                const showEditDiary = dayContextMenu.selectedMDContentPath !== ""
                const showEditStatus = monthGrid.hasStatusForDay(dayContextMenu.selectedDay)

                if (showEditDiary) {
                    var d = cmpEditDiary.createObject(null)
                    editItemMenu.addItem(d)
                    __dynItems.push(d)
                }
                if (showEditStatus) {
                    var s = cmpEditStatus.createObject(null)
                    editItemMenu.addItem(s)
                    __dynItems.push(s)
                }
                if (!showEditDiary && !showEditStatus) {
                    var n = cmpNoItem.createObject(null)
                    editItemMenu.addItem(n)
                    __dynItems.push(n)
                }
            }
        }

        Menu {
            id: showItemMenu
            title: qsTr("Show Item")

            property bool hasDiaryItems: dayContextMenu.selectedMDContentPath !== ""

            property var __dynItems: []
            onAboutToShow: {
                for (var i = 0; i < __dynItems.length; i++) {
                    var obj = __dynItems[i]
                    if (obj) {
                        try { showItemMenu.removeItem(obj) } catch(e) {}
                        try { obj.destroy() } catch(e) {}
                    }
                }
                __dynItems = []

                const showDiary = showItemMenu.hasDiaryItems
                const showStatus = monthGrid.hasStatusForDay(dayContextMenu.selectedDay)

                if (showDiary) {
                    var d = cmpShowDiary.createObject(null)
                    showItemMenu.addItem(d)
                    __dynItems.push(d)
                }

                if (showStatus) {
                    var s = cmpShowStatus.createObject(null)
                    showItemMenu.addItem(s)
                    __dynItems.push(s)
                } 

                if (!showDiary && !showStatus) {
                    var n = cmpNoItem.createObject(null)
                    showItemMenu.addItem(n)
                    __dynItems.push(n)
                }
            }
        }
    }
}