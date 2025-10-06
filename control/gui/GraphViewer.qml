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
    property int fillter: 0
    property string to: ""
    property string from: ""

    property var graphData: ({}) // { start, end, days:[ {date, diary_exists, status:{...}} ] }

    signal requestGraphData(string workspace, int scope, int fillter, string to, string from)
 
    Component.onCompleted: {
        console.log("GraphViewer.qml" + workspace)    
        requestGraphData(workspace, scope, fillter, to, from);
    }

    onGraphDataChanged: {
        console.log("GraphViewer.qml graphData changed")
        updateGraph();
    }

    onScopeChanged: {
        requestGraphData(workspace, scope, fillter, to, from);
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

           if (!graphData || typeof graphData != "object") {
               console.log("graphData is not an object")
               return;
           }
           var days = graphData.days
           console.log("graphData.days:", days)
           console.log("graphData.days type:", typeof days)
           if (!days) {
               console.warn("graphData.days is null or undefined")
               return;     
           }
           
           sleepSeries.clear();
           wakeUpSeries.clear();
           moodSeries.clear();
           temperatureSeries.clear();
        
           var sleepPoints = []
           var wakeUpPoints = []
           var moodPoints = []
           var temperaturePoints = []
           for (var i = 0; i < days.length; ++i) {
                var day = days[i]
                console.log("Day", i, ":", JSON.stringify(day))
            
                if (!day || !day.status) continue;

                var status = day.status
                var time = new Date(day.date + "T00:00:00").getTime()

               if (status.sleep_time !== undefined) {
                    var sleepTime = parseTime(status.sleep_time)
                    if (isFinite(sleepTime)) {
                        sleepPoints.push({ x: time, y: sleepTime, yPoint: formatHHmm(sleepTime) })
                    }     
               }

               if (status.wake_up_time !== undefined) {
                    var wakeUpTime = parseTime(status.wake_up_time)
                    if (isFinite(wakeUpTime)) {
                        wakeUpPoints.push({ x: time, y: wakeUpTime, yPoint: formatHHmm(wakeUpTime) })
                    }     
               }

              if (scope === 0) {
                    if (status.mood !== undefined) {
                        var mood = Number(status.mood)
                        if (isFinite(mood)) {
                            moodPoints.push({ x: time, y: mood, yPoint: String(mood) })
                        }     
                    }

                   if (status.temperature !== undefined) {
                        var temperature = Number(status.temperature)
                        if (isFinite(temperature)) {
                            temperaturePoints.push({ x: time, y: temperature, yPoint: String(temperature) })
                        }     
                    }
               }
           }

        console.log("sleepPoints counts:", sleepPoints.length)

        for (var a = 0; a < sleepPoints.length; ++a) sleepSeries.append(sleepPoints[a].x, sleepPoints[a].y)
        for (var b = 0; b < wakeUpPoints.length;  ++b) wakeUpSeries.append(wakeUpPoints[b].x, wakeUpPoints[b].y)
        if (scope === 0) {
            for (var c = 0; c < moodPoints.length; ++c) moodSeries.append(moodPoints[c].x, moodPoints[c].y)
            for (var d = 0; d < temperaturePoints.length; ++d) temperatureSeries.append(temperaturePoints[d].x, temperaturePoints[d].y)
        }

        var allPoints = (scope === 0)
            ? sleepPoints.concat(wakeUpPoints, moodPoints, temperaturePoints)
            : sleepPoints.concat(wakeUpPoints)

        if (allPoints.length > 0) {
            var minX = allPoints[0].x
            var maxX = allPoints[0].x

            for (var k = 1; k < allPoints.length; ++k) {
                if (allPoints[k].x < minX) minX = allPoints[k].x
                if (allPoints[k].x > maxX) maxX = allPoints[k].x
            }
            if (minX === maxX) {
                var oneDay = 24 * 3600 * 1000
                axisX.min = new Date (minX - oneDay)
                axisX.max = new Date (maxX + oneDay)
            } else {
                axisX.min = new Date (minX)
                axisX.max = new Date (maxX)
            }
        }

        if (sleepPoints.length || wakeUpPoints.length) {
            var allSW = sleepPoints.concat(wakeUpPoints)
            var smin = 24, smax = 0
            for (var m = 0; m < allSW.length; ++m) {
                if (allSW[m].y < smin) smin = allSW[m].y
                if (allSW[m].y > smax) smax = allSW[m].y
            }
            var swRange = clampRange1pt(Math.max(0, Math.floor(smin) - 1),
                                        Math.min(24, Math.ceil(smax) + 1), 1, 2)
            axisSleep.min = Math.max(0, swRange.min)
            axisSleep.max = Math.min(24, swRange.max)
        }

        if (scope === 0 &&moodPoints.length) {
            var mmin = moodPoints[0].y
            var mmax = moodPoints[0].y
            for (var m = 1; m < moodPoints.length; ++m) {
                if (moodPoints[m].y < mmin) mmin = moodPoints[m].y
                if (moodPoints[m].y > mmax) mmax = moodPoints[m].y
            }
            var moodRange = clampRange1pt(mmin, mmax, 0.5, 1)
            axisMood.min = Math.floor(Math.min(0, moodRange.min))
            axisMood.max = Math.ceil(Math.max(5, moodRange.max))
        }

        if (scope ===0 && temperaturePoints.length) {
            var tmin = temperaturePoints[0].y
            var tmax = temperaturePoints[0].y
            for (var t = 1; t < temperaturePoints.length; ++t) {
                if (temperaturePoints[t].y < tmin) tmin = temperaturePoints[t].y
                if (temperaturePoints[t].y > tmax) tmax = temperaturePoints[t].y
            }
            var pad = 0.3
            axisTemp.min = Math.floor(Math.min(34, tmin - pad))
            axisTemp.max = Math.ceil(Math.max(38, tmax + pad))
        }

        moodSeries.visible        = (scope === 0 && moodPoints.length > 0)
        temperatureSeries.visible = (scope === 0 && temperaturePoints.length > 0)
        axisMood.visible          = (scope === 0)
        axisTemp.visible          = (scope === 0)
    }

    ChartView {
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
    }

}
