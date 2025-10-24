pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material
import QtCharts

Page {

    id: graphViewerPage
    title: "Graph Viewer"
    
    property string workspace: ""
    property int scope: 0 // 0=ALL, 1=Sleep/Wake
    property int filter: 0
    property string to: ""
    property string from: ""

    property var chartConfigs: [
        { title: "Sleep", accessor: "sleep_time", color: "#FF6B6B", type: "time", yRange: [0, 23] },
        { title: "Wake", accessor: "wake_up_time", color: "#4ECDC4", type: "time", yRange: [0, 23] },
        { title: "Mood", accessor: "mood", color: "#1A535C", type: "number", yRange: [0, 5] },
        { title: "Temp", accessor: "temperature", color: "#D0021B", type: "number", yRange: [35, 45] }
    ]

    property var seriesRegistry: ({})

    property var graphData: ({}) // { start, end, days:[ {date, diary_exists, status:{...}} ] }

    signal requestGraphData(string workspace, int scope, int filter, string to, string from)
 
    Component.onCompleted: {
        console.log("GraphViewer.qml" + workspace)    
        requestGraphData(workspace, scope, filter, to, from);
    }

    onGraphDataChanged: {
        console.log("GraphViewer.qml graphData changed")
        updateGraph();
    }

    onScopeChanged: {
        requestGraphData(workspace, scope, filter, to, from);
    }

    function toJsArray(list) {
        if (list && typeof list.length == "number") {
        
            try {
                return JSON.parse(JSON.stringify(list))    
            } catch(e) {
                return []
            }
        }
    }

    function parseTime(time) {
        if (time === "" || time === null || time === undefined) return NaN
        var  num = Number(time)
        if (!isFinite(num)) return NaN;
        var hor = Math.floor(num)
        var min = Math.round((num - hor) * 100)
        if (min < 0) min = 0;
        if (min > 59) min = 59;
        return hor + min/ 60
    }

    function clampRange1pt(minVal, maxVal, pad, minSpan) {
        if (minVal === maxVal) {
            return { min: minVal - pad, max: maxVal + pad }
        }
        if ((maxVal - minVal) < minSpan)
            return { min: minVal, max: minVal + minSpan }
        return { min: minVal, max: maxVal }
    }

    function formatHHmm(time) {
        if (!isFinite(time)) return ""
        var hor = Math.floor(time)
        var min = Math.round((time - hor) * 60)
        if (min === 60) { hor += 1; min= 0}
        return String(hor).padStart(2, '0') + ":" + String(min).padStart(2, '0')
    }

    function updateGraph() {
        if (!graphData || typeof graphData !== "object") {
            console.log("graphData is not an object")
            return
        }
        var days = graphData.days
        console.log("graphData.days:", days)
        console.log("graphData.days type:", typeof days)
        if (!days) {
            console.warn("graphData.days is null or undefined")
            return
        }

        var keys = Object.keys(seriesRegistry)
        if (!keys.length)
            return

        var oneDayMs = 24 * 3600 * 1000

        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i]
            var entry = seriesRegistry[key]
            if (!entry || !entry.series)
                continue

            var cfg = entry.config || null
            var series = entry.series
            var axisX = entry.axisX
            var axisY = entry.axisY

            series.clear()

            var points = []
            for (var d = 0; d < days.length; ++d) {
                var day = days[d]
                console.log("Day", d, ":", JSON.stringify(day))
                if (!day || !day.status)
                    continue
                if (day.status[key] === undefined)
                    continue

                var rawValue = day.status[key]
                if (typeof rawValue === "string")
                    rawValue = rawValue.trim()
                var value = (cfg && cfg.type === "time") ? parseTime(rawValue) : parseFloat(rawValue)
                if (!isFinite(value))
                    continue
                if (cfg && cfg.yRange && (value < cfg.yRange[0] || value > cfg.yRange[1]))
                    continue

                var timeStamp = new Date(day.date + "T00:00:00").getTime()
                points.push({ x: timeStamp, y: value })
            }

            if (!points.length) {
                if (cfg && cfg.yRange) {
                    axisY.min = cfg.yRange[0]
                    axisY.max = cfg.yRange[1]
                }
                continue
            }

            points.sort(function(a, b) { return a.x - b.x })

            var minX = points[0].x
            var maxX = points[0].x
            var minY = points[0].y
            var maxY = points[0].y

            for (var p = 1; p < points.length; ++p) {
                if (points[p].x < minX) minX = points[p].x
                if (points[p].x > maxX) maxX = points[p].x
                if (points[p].y < minY) minY = points[p].y
                if (points[p].y > maxY) maxY = points[p].y
            }

            if (minX === maxX) {
                axisX.min = new Date(minX - oneDayMs)
                axisX.max = new Date(maxX + oneDayMs)
            } else {
                axisX.min = new Date(minX)
                axisX.max = new Date(maxX)
            }

            if (cfg && cfg.yRange && cfg.yRange.length === 2) {
                axisY.min = cfg.yRange[0]
                axisY.max = cfg.yRange[1]
            } else if (cfg && cfg.type === "time") {
                var swRange = clampRange1pt(Math.max(0, Math.floor(minY) - 1),
                                            Math.min(24, Math.ceil(maxY) + 1), 1, 2)
                axisY.min = Math.max(0, swRange.min)
                axisY.max = Math.min(24, swRange.max)
            } else if (key === "mood") {
                var moodRange = clampRange1pt(minY, maxY, 0.5, 1)
                axisY.min = Math.floor(Math.min(0, moodRange.min))
                axisY.max = Math.ceil(Math.max(5, moodRange.max))
            } else {
                axisY.min = Math.floor(minY)
                axisY.max = Math.ceil(maxY)
            }

            for (var s = 0; s < points.length; ++s) {
                series.append(points[s].x, points[s].y)
            }
        }
    }

    function registerSeries(key, series, axisX, axisY, config) {
        seriesRegistry[key] = { series: series, axisX: axisX, axisY: axisY, config: config }
        updateGraph()
    }

    function unregisterSeries(key) {
        delete seriesRegistry[key]
    }

    GridLayout {
        id: chartContainer
        anchors.fill: parent
        columns: 2
        rowSpacing: 16
        columnSpacing: 16
        Repeater {
            model: scope === 0 ? chartConfigs : chartConfigs.slice(0, 2)
            delegate: ChartView {
                required property var modelData
                Layout.columnSpan: 1
                Layout.fillWidth: true
                Layout.preferredWidth: chartContainer.width / 2 - chartContainer.columnSpacing
                Layout.minimumWidth: Layout.preferredWidth
                Layout.maximumWidth: Layout.preferredWidth
                Layout.preferredHeight: 280
                antialiasing: true
                legend.visible: false
                title: modelData.title
                theme: ChartView.ChartThemeBrownSand

                DateTimeAxis {
                    id: axisX
                    format: "MM-dd"
                    labelsAngle: -45
                    tickCount: 6
                }

                ValueAxis {
                    id: axisY
                    min: modelData.yRange ? modelData.yRange[0] : 0
                    max: modelData.yRange ? modelData.yRange[1] : 10
                }

                LineSeries {
                    id: lineSeries
                    axisX: axisX
                    axisY: axisY
                    color: modelData.color
                    width: 2
                    pointsVisible: true
                }

                Component.onCompleted: graphViewerPage.registerSeries(
                                           modelData.accessor, lineSeries, axisX, axisY, modelData)
                Component.onDestruction: graphViewerPage.unregisterSeries(modelData.accessor)

                PropertyAnimation on opacity {
                    duration: 120
                    from: 0
                    to: 1
                }
            }
        }
    }

    function populateSeries(key) {
        var series = null;
        switch(key) {
            case "sleep_time":
                series = sleepSeries;
                break;
            case "wake_up_time":
                series = wakeUpSeries;
                break;
            case "mood":
                series = moodSeries;
                break;
            case "temperature":
                series = temperatureSeries;
                break;
            default:
                console.warn("Unknown key for series:", key);
                return;
        }

        series.clear();

        if (!graphData || !graphData.days) {
            console.warn("No graph data available");
            return;
        }

        for (var i = 0; i < graphData.days.length; ++i) {
            var day = graphData.days[i];
            if (!day || !day.status || day.status[key] === undefined) continue;

            var value = day.status[key];
            var time = new Date(day.date + "T00:00:00").getTime();

            if (key === "sleep_time" || key === "wake_up_time") {
                value = parseTime(value);
            } else {
                value = Number(value);
            }

            if (isFinite(value)) {
                series.append(time, value);
            }
        }
    }

    /* ChartView {
        id: chartView
        anchors.fill: parent
        antialiasing: true
        theme: ChartView.ChartThemeBrownSand
        title: "Graph Viewer"
        legend.visible: true
        backgroundColor: "#F5F5F5"

       DateTimeAxis {
            id: axisX
            format: "MM-dd"
            titleText: "Date"
            labelsAngle: -45
            tickCount: 6
        }

        ValueAxis {
            id: axisSleep
            titleText: "Sleep/Wake (h)"
            min: 0
            max: 24
            gridVisible: true
        }

        ValueAxis {
            id: axisMood
            titleText: "Mood"
            visible: scope === 0
            min: 0
            max: 5
            gridVisible: false
            labelsVisible: true
        }

         ValueAxis {
            id: axisTemp
            titleText: "Temp (°C)"
            visible: scope === 0
            min: 34
            max: 40
            gridVisible: false
            labelsVisible: true
        }

        LineSeries {
            id: sleepSeries
            name: "Sleep"
            axisX: axisX
            axisY: axisSleep
            width: 2
            color: "#FF6B6B"
            pointsVisible: true
        }

        LineSeries {
            id: wakeUpSeries
            name: "Wake"
            axisX: axisX
            axisY: axisSleep
            width: 2
            color: "#4ECDC4"
            pointsVisible: true
        }

        LineSeries {
            id: moodSeries
            name: "Mood"
            axisX: axisX
            axisY: axisMood
            width: 2
            color: "#1A535C"
            pointsVisible: true
            visible: scope === 0
        }

        LineSeries {
            id: temperatureSeries
            name: "Temp"
            axisX: axisX
            axisY: axisTemp
            width: 2
            color: "#D0021B"
            pointsVisible: true
            visible: scope === 0
        }
    } */

}
