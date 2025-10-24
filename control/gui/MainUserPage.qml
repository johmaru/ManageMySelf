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
    property var activityBarThemeSettings: undefined

    property string _pendingDiaryJson: ""
    property string _pendingStatusJson: ""
    property string _pendingMargedJson: ""
    property bool _hasPendingMerged: false
    property bool _diaryArrived: false
    property bool _statusArrived: false

    property var monthGridRef: null

    signal showError(string message)
    signal requestGetUserName(string path)
    signal backRequested()
    signal requestCreateDiary(int year, int month,int day,string title, string path)
    signal requestCreateStatus(int year, int month, int day, string path)
    signal requestMonthUserDiarySqlData(int year, int month, string path)
    signal requestMonthUserStatusData(int year, int month, string path)
    signal updateMonthGridData(string jsonString)
    signal updateSearchResults(string jsonString)
    signal requestNavigateMarkdownEditor(string contentPath)
    signal requestNavigateMarkdownViewer(string contentPath)
    signal requestStatusEditor(int year, int month, int day, string jsonString, string path, int mode)
    signal requestSearch(string query, string scope, bool caseSensitive, bool useRegex, string path)
    signal requestCreateNewWindowForGraph(string path, int Scope, int Fillter, string toStr, string fromStr)

    ListModel { id: condModel; Component.onCompleted: append({ field: "title", op: "contains", value: "", logic: "AND" }) }
    ListModel { id: searchModel }

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
        var mg = mainUserPage.monthGridRef
        if (mg) {
            mg.updateMonthData(jsonString)
        } else {
            _pendingMargedJson = jsonString
            _hasPendingMerged = true
            Qt.callLater(function() {
                var mg2 = mainUserPage.monthGridRef
                if (mg2 && _hasPendingMerged) {
                    mg2.updateMonthData(_pendingMargedJson)
                    _pendingMargedJson = ""
                    _hasPendingMerged = false
                }
            })
        }
    }


    Component.onCompleted: {
        console.log("Today's date:", new Date())
        console.log("JavaScript getMonth():", new Date().getMonth())
        console.log("Current month property:", currentMonth)
        var mg = mainUserPage.monthGridRef
        if (!mg) {
            console.error("MonthGrid not found")
        } else {
            console.log("MonthGrid month:", mg.month)
        }
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
        var mg = mainUserPage.monthGridRef
        if (mg) {
            mg.loadMonthData();
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

    function applySearchResults(jsonString) {
        var obj
        try {
            obj = JSON.parse(jsonString)
        } catch (e) {
            console.error("Invalid search results JSON:", e)
            searchModel.clear()
            return
        }

        var rows = []
        if (Array.isArray(obj)) {
            rows = obj
        } else if (Array.isArray(obj.results)) {
            rows = obj.results
        } else if (Array.isArray(obj.groups)) {
            for (var gi = 0; gi < obj.groups.length; gi++) {
                var g = obj.groups[gi]
                var items = g.items || []
                for (var ii = 0; ii < items.length; ii++) {
                    var it = items[ii]
                    if (!it.createdAt && g.date) it.createdAt = g.date + " 00:00:00"
                    rows.push(it)
                }
            }
        } else {
            rows = []
        }

            searchModel.clear()
            console.log("applySearchResults: rows=", rows.length)
        for (var i = 0; i < rows.length; i++) {
            var r = rows[i]
            var item = {
                type: r.type || "",
                title: r.title || "",
                contentPath: r.contentPath || "",
                createdAt: r.createdAt || r.date || ""
            }
            if (r.status) {
                if (r.status.mood !== null && r.status.mood !== undefined) item.mood = r.status.mood
                if (r.status.freeMoodText !== null && r.status.freeMoodText !== undefined) item.freeMoodText = r.status.freeMoodText
                if (r.status.sleepTime !== null && r.status.sleepTime !== undefined) item.sleepTime = r.status.sleepTime
                if (r.status.wakeUpTime !== null && r.status.wakeUpTime !== undefined) item.wakeUpTime = r.status.wakeUpTime
                if (r.status.temperature !== null && r.status.temperature !== undefined) item.temperature = r.status.temperature
            }
            item.raw = r
            searchModel.append(item)
        }
    }

    onUpdateSearchResults: function(jsonString) {
        mainUserPage.applySearchResults(jsonString)
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
            var mg = mainUserPage.monthGridRef
            if (!mg) {
                mainUserPage.showError(qsTr("MonthGrid not found"))
                return
            }
            mainUserPage.requestCreateDiary(mg.year, mg.month + 1, mg.currentDay, title, mainUserPage.workspacePath)

            Qt.callLater(function() {
                mg.loadMonthData()
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

    function handleActivityChange(key) {
                    switch (key) {
                        case "toggle":
                            sidePanel.isSelected = !sidePanel.isSelected
                            break;
                        case "search":
                             sidePanel.isSelected = true
                             break;
                        case "graph":
                            if (sidePanel.isSelected) sidePanel.isSelected = false
                            else sidePanel.isSelected = true
                            break;
                        case "settings":
                            if (sidePanel.isSelected) sidePanel.isSelected = false
                            else sidePanel.isSelected = true
                            break;
                        case "back":
                            mainUserPage.backRequested()
                            break;
                    }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 0

                ActivityBar {
                    id: activityLoader
                    Layout.preferredWidth: 120
                    Layout.fillHeight: true
                    mode: 0
                    scene: ActivityBar.Scene.MainUserPage
                    theme: mainUserPage.themeSettings
                    ab_theme: mainUserPage.activityBarThemeSettings
                    // onCurrentIndexChanged: statusEditorPage.handleActivityChange(currentIndex)
                    onActivated: function(key) { mainUserPage.handleActivityChange(key) }
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
                        currentIndex: activityLoader.currentKey === "settings" ? 1 
                                    : activityLoader.currentKey === "search" ? 2
                                    : activityLoader.currentKey === "graph" ? 3
                                    : 0

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

                        Column {
                            spacing: 8
                            padding: 8
                            Label { text: qsTr("Search") }
                            
                            ComboBox {
                                id: searchMode
                                model: [ qsTr("Expression"), qsTr("Builder")]
                                currentIndex: 0
                                    width: searchResults ? searchResults.width : 0
                            }

                            Loader {
                                id: searchUiLoader
                                sourceComponent: searchMode.currentIndex === 0 ? exprSearchComp : builderSearchComp
                            }

                            ListView {
                                id: searchResults
                                height: 240
                                Layout.fillWidth: true
                                model: searchModel
                                delegate: ItemDelegate {
                                    required property int index
                                    width: ListView.view ? ListView.view.width : 0

                                    property var __row: (index >= 0 && index < searchModel.count) ? searchModel.get(index) : ({})
                                    text: ((__row && __row.type) || "") + " " + ((__row && __row.createdAt) || "") + " " + ((__row && __row.title) || "")
                                    onClicked: {
                                        var row = (index >= 0 && index < searchModel.count) ? searchModel.get(index) : null
                                        if (row && row.contentPath && row.contentPath !== "") {
                                            mainUserPage.requestNavigateMarkdownViewer(row.contentPath)
                                        } else {
                                            mainUserPage.showError(qsTr("No content path available"))
                                        }
                                    }
                                }
                                visible: searchModel.count > 0
                            }
                             Label {
                                id: resultHint
                                text: searchModel.count === 0 ? qsTr("No results") : ""
                                visible: searchModel.count === 0
                            }
                        }
                     
                    Item {
                        id: graphContainer
                        Layout.fillWidth: true

                        property int graphScope: graphScopeComboBox.currentIndex  // 0: with Diary, 1: with Status
                        property int graphFilter: graphFilterComboBox.currentIndex  // 0: All, 1: Mood

                        Column {
                        id: graphColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 8
                        padding: 8

                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Graph") }

                        Item{height:80}

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 1
                            color: Material.dividerColor
                            opacity: 0.7
                        }
                        
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("day range") }
                        
                        Row{
                            spacing: 8
                           
                            Label { anchors.verticalCenter: parent.verticalCenter; text: qsTr("From") }

                            Row {
                                id: prevDayPicker
                                spacing: 6

                                property int year: mainUserPage.currentYear
                                property int month: mainUserPage.currentMonth
                                property int day: new Date().getDate()
                                property date dateValue: new Date(year, month - 1, day)
                                property string yyyy_mm_dd: Qt.formatDate(dateValue, "yyyy-MM-dd")
                                function daysInMonth(y, m) { return new Date(y, m, 0).getDate() }  // 数値で返す

                                TextField {
                                    id: prevYYYYmmddField
                                    width: 90
                                    placeholderText: qsTr("yyyy-MM-dd or yyyymmdd")
                                    inputMethodHints: Qt.ImhDigitsOnly
                                    text: prevDayPicker.yyyy_mm_dd
                                    property bool _updating: false

                                    onEditingFinished: {
                                        if (_updating) return
                                        _updating = true
                                        const s = text.trim()
                                        let y, m, d
                                        if (s.indexOf('-') > 0) {
                                            const parts = s.split('-')
                                            if (parts.length === 3) {
                                                y = parseInt(parts[0]); m = parseInt(parts[1]); d = parseInt(parts[2])
                                            }
                                        } else if (s.length === 8) {
                                            y = parseInt(s.slice(0,4)); m = parseInt(s.slice(4,6)); d = parseInt(s.slice(6,8))
                                        }
                                        if (!isNaN(y) && !isNaN(m) && !isNaN(d)) {
                                            const dim = prevDayPicker.daysInMonth(y, m)
                                            if (d >= 1 && d <= dim) {
                                                prevDayPicker.year = y
                                                prevDayPicker.month = m
                                                prevDayPicker.day = d
                                                prevDayPicker.dateValue = new Date(y, m - 1, d)
                                                text = Qt.formatDate(prevDayPicker.dateValue, "yyyy-MM-dd")
                                            }
                                        }
                                        _updating = false
                                    }

                                    Connections {
                                        target: prevDayPicker
                                        function onDateValueChanged() {
                                            if (!prevYYYYmmddField._updating && !prevYYYYmmddField.activeFocus) {
                                                prevYYYYmmddField._updating = true
                                                prevYYYYmmddField.text = Qt.formatDate(prevDayPicker.dateValue, "yyyy-MM-dd")
                                                prevYYYYmmddField._updating = false
                                            }
                                        }
                                    }
                                }
                            }

                            Label { anchors.verticalCenter: parent.verticalCenter; text: qsTr("To") }

                            Row {
                                id: nextDayPicker
                                spacing: 6

                                property int year: mainUserPage.currentYear
                                property int month: mainUserPage.currentMonth
                                property int day: new Date().getDate()
                                property date dateValue: new Date(year, month - 1, day)
                                property string yyyy_mm_dd: Qt.formatDate(dateValue, "yyyy-MM-dd")
                                function daysInMonth(y, m) { return new Date(y, m, 0).getDate() }

                                TextField {
                                    id: nextYYYYmmddField
                                    width: 90
                                    placeholderText: qsTr("yyyy-MM-dd or yyyymmdd")
                                    inputMethodHints: Qt.ImhDigitsOnly
                                    text: nextDayPicker.yyyy_mm_dd
                                    property bool _updating: false

                                    onEditingFinished: {
                                        if (_updating) return
                                        _updating = true
                                        const s = text.trim()
                                        let y, m, d
                                        if (s.indexOf('-') > 0) {
                                            const parts = s.split('-')
                                            if (parts.length === 3) {
                                                y = parseInt(parts[0]); m = parseInt(parts[1]); d = parseInt(parts[2])
                                            }
                                        } else if (s.length === 8) {
                                            y = parseInt(s.slice(0,4)); m = parseInt(s.slice(4,6)); d = parseInt(s.slice(6,8))
                                        }
                                        if (!isNaN(y) && !isNaN(m) && !isNaN(d)) {
                                            const dim = nextDayPicker.daysInMonth(y, m)
                                            if (d >= 1 && d <= dim) {
                                                nextDayPicker.year = y
                                                nextDayPicker.month = m
                                                nextDayPicker.day = d
                                                nextDayPicker.dateValue = new Date(y, m - 1, d)
                                                text = Qt.formatDate(nextDayPicker.dateValue, "yyyy-MM-dd")
                                            }
                                        }
                                        _updating = false
                                    }

                                    Connections {
                                        target: nextDayPicker
                                        function onDateValueChanged() {
                                            if (!nextYYYYmmddField._updating && !nextYYYYmmddField.activeFocus) {
                                                nextYYYYmmddField._updating = true
                                                nextYYYYmmddField.text = Qt.formatDate(nextDayPicker.dateValue, "yyyy-MM-dd")
                                                nextYYYYmmddField._updating = false
                                            }
                                        }
                                    }
                                }
                            }
                        }

                       Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Scope") }

                       ComboBox {
                            id: graphScopeComboBox
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 200
                            model: [ qsTr("All Metrics"), qsTr("Sleep & Wake Only") ]
                            currentIndex: 0

                            onCurrentIndexChanged: {
                                graphContainer.graphScope = currentIndex                            
                            }                 
                       }

                       Label { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Filter")}
                       
                       ComboBox {
                            id: graphFilterComboBox
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 200
                            model: [ qsTr("All"), qsTr("Mood") ]
                            currentIndex: 0

                            onCurrentIndexChanged: {
                                graphContainer.graphFilter = currentIndex
                            }
                       }

                       Button {
                            
                            id: loadGraphButton

                           text: qsTr("Load Graph")
                           anchors.horizontalCenter: parent.horizontalCenter
                           onClicked: {
                               var mg = mainUserPage.monthGridRef
                               if (!mg) {
                                   mainUserPage.showError(qsTr("MonthGrid not found"))
                                   return
                               }
                               var fromDate = prevDayPicker.dateValue
                               var toDate = nextDayPicker.dateValue
                               if (fromDate > toDate) {
                                   mainUserPage.showError(qsTr("From date must be earlier than To date"))
                                   return
                               }
                               var scope = graphContainer.graphScope === 0 ? "diary" : "status"
                               var filter = graphContainer.graphFilter === 0 ? "all" : "mood"
                               var fromStr = Qt.formatDate(fromDate, "yyyy-MM-dd")
                               var toStr = Qt.formatDate(toDate, "yyyy-MM-dd")
                               console.log("Requesting graph data from", fromStr, "to", toStr, "scope:", scope, "filter:", filter)
                               mainUserPage.requestCreateNewWindowForGraph(mainUserPage.workspacePath, graphContainer.graphScope, graphContainer.graphFilter, toStr, fromStr)
                           }

                       }
                        

                    }
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
                        sourceComponent: mainUserPageComponent
                    }
                }
            }

    Component {
        id: mainUserPageComponent
        ColumnLayout {
        id: mainRoot
        property alias monthGridRef: monthGrid

        anchors.centerIn: parent
        spacing: 10

        Component.onCompleted: {
            mainUserPage.monthGridRef = monthGrid
        }

        ToolButton {
            id: datePickerButton
            text: "日付選択"
            onClicked: datePickerDialog.open()
        }

        GridView {
        id: monthGrid

        cellWidth: 40
        cellHeight: 40

        implicitWidth: cellWidth * 7
        implicitHeight: cellHeight * 6
        Layout.alignment: Qt.AlignHCenter
        
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
                            var mg = mainUserPage.monthGridRef
                            dayContextMenu.selectedHasStatus = mg ? mg.hasStatusForDay(parent.day) : false
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
            if (mainUserPage._hasPendingMerged) {
                monthGrid.updateMonthData(mainUserPage._pendingMargedJson)
                mainUserPage._pendingMargedJson = ""
                mainUserPage._hasPendingMerged = false
            }
            loadMonthData()
        }
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
                var mg = mainUserPage.monthGridRef
                if (!mg) {
                    mainUserPage.showError(qsTr("MonthGrid not found"))
                    return
                }
                var selectedDate = mainUserPage.currentYear + "-" + ("0" + mainUserPage.currentMonth).slice(-2) + "-" + ("0" + mg.currentDay).slice(-2)
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
                var mg = mainUserPage.monthGridRef
                if (!mg) {
                    mainUserPage.showError(qsTr("MonthGrid not found"))
                    return
                }
                var selectedDate = mainUserPage.currentYear + "-" + ("0" + mainUserPage.currentMonth).slice(-2) + "-" + ("0" + mg.currentDay).slice(-2)
                if (selectedDate === today) {
                    mainUserPage.requestCreateStatus(mainUserPage.currentYear, mainUserPage.currentMonth, mg.currentDay, mainUserPage.workspacePath)
                } else {
                    console.warn("Cannot create status for past days:", mg.currentDay)
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
        property bool selectedHasStatus: false

        Menu {
            id: createItemMenu
            title: qsTr("Create Item")

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

                const mg = mainUserPage.monthGridRef
                const showCreateDiary = (dayContextMenu.selectedMDContentPath == "")
                const showCreateStatus = !dayContextMenu.selectedHasStatus

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
                var mg = mainUserPage.monthGridRef
                const showEditStatus = dayContextMenu.selectedHasStatus

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

                const mg = mainUserPage.monthGridRef
                const showDiary = showItemMenu.hasDiaryItems
                const showStatus = dayContextMenu.selectedHasStatus

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

    Component {
    id: exprSearchComp
    Column {
        spacing: 6

        TextField {
            id: queryField
            placeholderText: qsTr('e.g. title:"work" & mood>3 | date>=2025-09-01')
            width: 240
            onAccepted: searchBtn.clicked()
        }

        Row {
            spacing: 6
            ComboBox {
                id: scopeBox
                model: [qsTr("All"), qsTr("Diary"), qsTr("Status")]
                width: 100
            }
            CheckBox { id: caseSensitiveBox; text: qsTr("Case") }
            CheckBox { id: regexBox; text: qsTr("Regex") }
            ToolButton {
                text: "?"
                onClicked: helpPopup.open()
            }
        }

        Row {
            spacing: 6
            Button {
                id: searchBtn
                text: qsTr("Search")
                onClicked: {
                    mainUserPage.requestSearch(
                        queryField.text,
                        scopeBox.currentText,
                        caseSensitiveBox.checked,
                        regexBox.checked,
                        mainUserPage.workspacePath
                    )
                }
            }
            Button {
                text: qsTr("Clear")
                onClicked: {
                    queryField.text = ""
                    searchModel.clear()
                }
            }
        }

        Popup {
            id: helpPopup
            x: 10; y: 10
            modal: false
            contentItem: Column {
                spacing: 4
                padding: 8
                Label { text: qsTr("Operators: & = AND, | = OR, <> = not equal, () grouping") }
                Label { text: qsTr("Comparisons: =, !=(<>), <, <=, >, >=") }
                Label { text: qsTr('Example: title:"work" & (mood>3 | temperature>=37)') }
            }
        }
    }
}

Component {
    id: builderSearchComp
    Column {
        spacing: 6

        Repeater {
            id: condRepeater
            model: condModel
            delegate: Row {
                id: condRow
                required property int index
                spacing: 6
                ComboBox {
                    id: fieldBox
                    width: 100
                    model: [ "title", "content", "date", "mood", "temperature" ]
                    textRole: "display"
                    onCurrentTextChanged: condModel.setProperty(condRow.index, "field", currentText)
                    Component.onCompleted: currentIndex = Math.max(0, fieldBox.model.indexOf((condModel.get(condRow.index).field) || "title"))
                }
                ComboBox {
                    id: opBox
                    width: 90
                    model: [ "=", "!=", "<", "<=", ">", ">=", "contains", "not contains" ]
                    onCurrentTextChanged: condModel.setProperty(condRow.index, "op", currentText)
                    Component.onCompleted: currentIndex = Math.max(0, opBox.model.indexOf((condModel.get(condRow.index).op) || "="))
                }
                TextField {
                    id: valueField
                    width: 120
                    text: (condModel.get(condRow.index).value) || ""
                    onTextChanged: condModel.setProperty(condRow.index, "value", text)
                }
                ComboBox {
                    id: logicBox
                    width: 70
                    model: [ "AND", "OR" ]
                    onCurrentTextChanged: condModel.setProperty(condRow.index, "logic", currentText)
                    visible: condRow.index < condModel.count - 1
                    Component.onCompleted: currentIndex = Math.max(0, logicBox.model.indexOf((condModel.get(condRow.index).logic) || "AND"))
                }
                ToolButton {
                    text: "🗑"
                    onClicked: condModel.remove(condRow.index)
                }
            }
        }

        Row {
            spacing: 6
            Button {
                text: qsTr("+ Condition")
                onClicked: condModel.append({ field: "title", op: "contains", value: "", logic: "AND" })
            }
            Button {
                text: qsTr("Search")
                onClicked: {
                    const parts = []
                    for (let i = 0; i < condModel.count; i++) {
                        const c = condModel.get(i)
                        const v = (c.op === "contains" || c.op === "not contains")
                            ? `"${c.value}"`
                            : c.value
                        parts.push(`${c.field} ${c.op} ${v}`)
                        if (i < condModel.count - 1) parts.push(c.logic === "OR" ? "|" : "&")
                    }
                    const query = parts.join(" ")
                    mainUserPage.requestSearch(query, "All", false, false, mainUserPage.workspacePath)
                }
            }
            Button {
                text: qsTr("Clear")
                onClicked: {
                    condModel.clear()
                    condModel.append({ field: "title", op: "contains", value: "", logic: "AND" })
                    searchModel.clear()
                }
            }
        }
    }
}
}