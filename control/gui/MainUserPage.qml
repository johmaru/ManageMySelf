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
    signal requestMonthUserDiarySqlData(int year, int month, string path)
    signal updateMonthGridData(string jsonString)
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
            
            diaryTitleField.text = ""
        }
        
        onRejected: {
            diaryTitleField.text = ""
        }
    }

    Dialog {
        id: datePickerDialog
        title: "日付を選択"
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

        MenuItem {
            text: qsTr("Edit")
            onTriggered: {
                console.log("Edit diary for day:", dayContextMenu.selectedDay);
            }
        }

        Menu {
        id: showItemMenu
        title: qsTr("Show Item")
        
        property bool hasItems: dayContextMenu.selectedMDContentPath !== ""
        
        MenuItem {
            text: qsTr("Show Diary")
            visible: showItemMenu.hasItems
            enabled: showItemMenu.hasItems
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
            visible: !showItemMenu.hasItems
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
        
        function loadMonthData() {
            mainUserPage.requestMonthUserDiarySqlData(mainUserPage.currentYear, mainUserPage.currentMonth, mainUserPage.workspacePath)
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
        }
    }
    }
}