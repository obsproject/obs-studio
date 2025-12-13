import QtQuick
import QtQuick.Shapes

Shape {
    id: connection
    
    // Geometry props
    required property real startX
    required property real startY
    required property real endX
    required property real endY
    
    // Style props
    property color strokeColor: "#5080FF" // Blue-ish
    property real strokeWidth: 3
    
    // Performance: Bound box optimization? 
    // Shape fills its parent usually, but we want it to be an overlay.
    // Actually, Shape should be an Item ideally covering the whole area if possible, 
    // OR distinct items if performance allows. For < 100 connections, individual shapes are fine.
    
    ShapePath {
        strokeColor: connection.strokeColor
        strokeWidth: connection.strokeWidth
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        
        startX: connection.startX
        startY: connection.startY
        
        PathCubic {
            x: connection.endX
            y: connection.endY
            
            // Control points for curvature
            property real dist: Math.abs(connection.endX - connection.startX) * 0.5
            control1X: connection.startX + dist
            control1Y: connection.startY
            control2X: connection.endX - dist
            control2Y: connection.endY
        }
    }
}
