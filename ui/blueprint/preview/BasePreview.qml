import QtQuick 2.15
import QtQuick.Controls 2.15

/**
 * BasePreview - Common container for 3D Viewports
 * 
 * Usage:
 * BasePreview {
 *     title: "Scene Preview"
 *     controller: myController
 * }
 */
Rectangle {
    id: root
    
    // Public Properties
    property string title: "Preview"
    property var controller: null // Reference to BasePreviewController C++ object
    
    // Style
    color: "black" // 3D viewport background
    border.color: "#95A5A6"
    border.width: 1
    
    // Header Overlay (Fade in on hover?)
    Rectangle {
        id: header
        height: 30
        width: parent.width
        anchors.top: parent.top
        color: "#AA000000" // Semi-transparent black
        visible: true
        
        Label {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            text: root.title
            color: "white"
            font.bold: true
        }
        
        // Active Indicator
        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            width: 10
            height: 10
            radius: 5
            color: (root.controller && root.controller.active) ? "#2ECC71" : "#E74C3C"
        }
    }
    
    // Resize Handler
    onWidthChanged: {
        if (root.controller) root.controller.resize(width, height)
    }
    onHeightChanged: {
        if (root.controller) root.controller.resize(width, height)
    }
    
    // Mouse Handling for Camera (Placeholder)
    MouseArea {
        anchors.fill: parent
        z: -1 // Behind potential overlays, but valid for background drag
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        
        onPositionChanged: {
            // Calculate delta
            var dx = mouse.x - lastX
            var dy = mouse.y - lastY
            
            if (root.controller) {
                if (mouse.buttons & Qt.LeftButton) {
                    root.controller.orbit(dx, dy)
                } else if (mouse.buttons & Qt.RightButton) {
                    root.controller.pan(dx, dy)
                }
            }
            
            lastX = mouse.x
            lastY = mouse.y
        }
        
        onPressed: {
            lastX = mouse.x
            lastY = mouse.y
        }
        
        onWheel: {
            if (root.controller) {
                // Determine delta sign and magnitude
                var delta = wheel.angleDelta.y / 120.0
                root.controller.zoom(delta)
            }
        }
        
        // Private state for drag calculation
        property real lastX: 0
        property real lastY: 0
    }
}
