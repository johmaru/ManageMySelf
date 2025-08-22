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
    signal requestCreateDiary(string title, string path)
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
                            createDiaryDialog.open()
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
        try {diaryObj = JSON.parse(diaryJson)} catch(e) {diaryObj = {}}
        try {statusObj = JSON.parse(statusJson)} catch(e) {statusObj = {}}

        var diaries = _normalizeDiaryArray(diaryObj)
         var statuses = []
        if (Array.isArray(statusObj.statuses)) statuses = statusObj.statuses
        else if (Array.isArray(statusObj.status)) statuses = statusObj.status

        var statusByDate = {}

        for (var i = 0; i < statuses.length; i++) {
            var s = statuses[i]
            if (s.createdAt) statusByDate[s.createdAt] = s
        }

        for (var j = 0; j < diaries.length; j++) {
            var d = diaries[j]
            var has = false
            if (d.createdAt && statusByDate[d.createdAt]) {
                d.status = statusByDate[d.createdAt]
                has = true
            }
            d.hasStatus = has
        }

        return { diaries: diaries }
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
            
            mainUserPage.requestCreateDiary(title, mainUserPage.workspacePath)

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
            property bool hasStatus: !monthGrid.hasStatusForDay(dayContextMenu.selectedDay)

            MenuItem {
                text: qsTr("Create Diary")
                visible: createItemMenu.hasDiaryItems
                enabled: createItemMenu.hasDiaryItems
                onTriggered: {
                    createDiaryDialog.open()
                }
            }

            MenuItem {
                    text: qsTr("Create Status")
                    visible: createItemMenu.hasStatus
                    enabled: createItemMenu.hasStatus
                    onTriggered: {
                        mainUserPage.requestCreateStatus(mainUserPage.workspacePath);
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
        property var monthData: ({})
        property int updateTrigger: 0
        
        model: 42  // 6週間分
        
        delegate: Rectangle {
            width: 40
            height: 40

            readonly property bool hasStatus: monthGrid.hasStatusForDay(day)
            
            property int day: {
                var firstDay = new Date(monthGrid.year, monthGrid.month, 1).getDay()
                var dayNumber = index - firstDay + 1
                return (dayNumber > 0 && dayNumber <= new Date(monthGrid.year, monthGrid.month + 1, 0).getDate()) ? dayNumber : 0
            }
            
            readonly property var dayDiaries: monthGrid.monthData[day] || []
            readonly property bool today: {
                var now = new Date()
                return day > 0 && 
                       now.getDate() === day && 
                       now.getMonth() === monthGrid.month && 
                       now.getFullYear() === monthGrid.year
            }
            
            color: {
            if (today) return Material.accent
            if (dayDiaries.length > 0) return Material.color(Material.LightBlue)
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
                color: Material.color(Material.Red)
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 2
                visible: parent.dayDiaries.length > 0
            }
            
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(mouse) {
                    if (parent.day === 0) return
                    
                    switch (mouse.button) {
                        case Qt.LeftButton:
                            var diaries = parent.dayDiaries
                            console.log("Date:", parent.day, "Diaries:", diaries.length)
                            if (diaries.length > 0) {
                                console.log("Diary titles:", diaries.map(d => d.title))
                            }
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
                var data = JSON.parse(jsonString)
                
                if (data.diaries) {
                    data = data.diaries
                }
                
                monthData = {}
                
                console.log("Processing", data.length, "diaries")
                console.log("Current year:", mainUserPage.currentYear, "MonthGrid month:", monthGrid.month)
                
                for (var i = 0; i < data.length; i++) {
                    var diary = data[i]
                    if (diary.createdAt) {
                        var parts = diary.createdAt.split(' ');
                        var dateParts = parts[0].split('-');
                        var timeParts = parts[1].split(':');
                        var dateObj = new Date(dateParts[0], dateParts[1] - 1, dateParts[2], timeParts[0], timeParts[1], timeParts[2]);
                        
                        console.log("Diary:", diary.title)
                        console.log("Created date parts:", dateParts)
                        console.log("Created dateObj:", dateObj)
                        console.log("dateObj year:", dateObj.getFullYear(), "month:", dateObj.getMonth(), "day:", dateObj.getDate())
                        
                        if (dateObj.getFullYear() === mainUserPage.currentYear && dateObj.getMonth() === monthGrid.month) {
                            var day = dateObj.getDate()
                            
                            console.log("Adding diary to day:", day)
                            
                            if (!monthData[day]) {
                                monthData[day] = []
                            }
                            monthData[day].push(diary)
                        } else {
                            console.log("Date mismatch - dateObj year:", dateObj.getFullYear(), "vs current:", mainUserPage.currentYear)
                            console.log("Date mismatch - dateObj month:", dateObj.getMonth(), "vs monthGrid:", monthGrid.month)
                        }
                    } else {
                        console.warn("Diary missing createdAt:", diary)
                    }
                }
                
                console.log("Final monthData:", JSON.stringify(monthData))
                monthDataChanged()
                updateTrigger++
                
            } catch (e) {
                console.error("JSON parse error:", e)
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