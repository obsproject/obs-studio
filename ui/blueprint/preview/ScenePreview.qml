import QtQuick 2.15
import QtQuick.Controls 2.15
import "."

/**
 * ScenePreview - Standard 2D or Monoscopic 3D Preview
 */
BasePreview {
    id: root
    title: "Scene Preview"
    
    // Properties specific to Scene Preview
    property bool showGrid: true
    property bool showStats: false

    // Control Overlay (Bottom)
    Rectangle {
        color: "#AA000000"
        height: 40
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: mouseArea.containsMouse // Only show on hover

        Row {
            anchors.centerIn: parent
            spacing: 20

            CheckBox {
                text: "Grid"
                checked: root.showGrid
                onCheckedChanged: root.showGrid = checked
                // Bind to controller if implemented
            }

            CheckBox {
                text: "Stats"
                checked: root.showStats
                onCheckedChanged: root.showStats = checked
            }
        }
    }
    
    // Grid Overlay (Placeholder for actual RHI rendering)
    // In reality, grid would be drawn by renderer. This is UI-only hint.
    Item {
        anchors.fill: parent
        visible: root.showGrid
        opacity: 0.2
        
        Repeater {
            model: parent.width / 50
            Rectangle {
                width: 1; height: parent.parent.height
                x: index * 50
                color: "white"
            }
        }
        Repeater {
            model: parent.height / 50
            Rectangle {
                height: 1; width: parent.parent.width
                y: index * 50
                color: "white"
            }
        }
    }
    
    // Stats Text
    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 15
        anchors.topMargin: 40 // Below header
        visible: root.showStats
        color: "#00FF00"
        font.family: "Monospace"
        text: "FPS: 60\nDrawCalls: 12\nTriangles: 4050"
    }

    // Hover handler
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton // Pass clicks through to BasePreview
        propagateComposedEvents: true
    }
}
