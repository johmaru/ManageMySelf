pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Page {

    id: graphViewerPage
    title: "Graph Viewer"
    
    property string workspace: ""
    property int scope: 0
    property int fillter: 0
    property string to: ""
    property string from: ""

    property var graphData: ({})

    signal requestGraphData(string workspace, int scope, int fillter, string to, string from)
 
    Component.onCompleted: {
        console.log("GraphViewer.qml" + workspace)    
        requestGraphData(workspace, scope, fillter, to, from);
    }

    onGraphDataChanged: {
        console.log("GraphViewer.qml graphData changed")
        updateGraph();
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
           var days = toJsArray(graphData.days)
           if (!days || typeof days.length !== "number") {
           
               console.warn("graphData.days is not an array")
               return;     
           }
        
           var sleepPoints = []
           for (var i = 0; i < graphData.days.length; i++) {
               var day = graphData.days[i];
               if (!day || !day.status) continue;

                var v = parseTime(day.status.sleep_time)

               if (!isNaN(v)) {
                   var t = new Date(day.date + "T00:00:00Z").getTime()
                   sleepPoints.push({x: t, y: v})
                }
           }

        console.log("sleepPoints counts:", sleepPoints.length)

        if (sleepPoints.length > 0) {
            var preview = []
            var n = Math.min(5, sleepPoints.length)
            for (var k = 0; k < n; k++) {
                preview.push({x: sleepPoints[k].x, y: formatHHmm(sleepPoints[k].y)} )
            } console.log("preview:", JSON.stringify(preview))
        }
        return;
    }

}
