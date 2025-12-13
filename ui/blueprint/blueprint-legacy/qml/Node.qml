import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Item {
    id: root
    
    // Properties required from model
    required property string nodeId
    property string nodeType: ""
    property real nodeX: 0
    property real nodeY: 0
    property bool is3D: false
    required property string nodeType
    
    // Position Binding
    x: nodeX
    y: nodeY
    width: 200
    height: 150
    z: dragArea.pressed ? 100 : (is3D ? 10 : 1) // Pop up when dragging or in 3D

    // 3D Pop Effect
    transform: Translate {
        y: (is3D && dragArea.containsMouse) ? -20 : 0
        Behavior on y { NumberAnimation { duration: 150 } }
    }
    // Dragging Logic
    property point dragStart: Qt.point(0,0)

    MouseArea {
        id: dragArea
        anchors.fill: root
        drag.target: root
        drag.axis: Drag.XAndY
        drag.smoothing: 0.1
        drag.threshold: 0
        
        onPressed: {
             root.z = 100 // Bring to front
             dragStart = Qt.point(root.x, root.y)
            // Set selection in Properties Panel
            propertiesViewModel.sourceUuid = nodeId
        }
        
        onReleased: {
            root.z = 1
            // Commit position to model
            nodeGraphModel.updatePosition(root.nodeId, root.x, root.y)
        }
        
        onPositionChanged: (mouse)=> {
            // Optional: emit live updates if needed, usage of drag.target handles visual move
        }
    }

    // Shadow using MultiEffect (Qt 6.5+)
    // Requires import QtQuick.Effects
    Rectangle {
        id: shadowSource
        anchors.fill: parent
        color: "black"
        radius: 8
        visible: false // Source hidden
    }
    
    MultiEffect {
        source: shadowSource
        anchors.fill: shadowSource
        shadowEnabled: true
        shadowColor: "#80000000"
        shadowBlur: 1.0
        shadowVerticalOffset: 4
    }

    // Main Body
    Rectangle {
        id: background
        anchors.fill: parent
        color: "#D02C2C32" // Semi-transparent dark
        radius: 8
        border.color: dragArea.pressed ? "#0078D7" : "#30FFFFFF"
        border.width: 1

        // Header
        Rectangle {
            id: header
            height: 32
            width: parent.width
            anchors.top: parent.top
            
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#45454A" }
                GradientStop { position: 1.0; color: "#2C2C32" } // Blend to body
            }
            
            // Clip top corners only? QML Rectangle radius applies to all. 
            // We can put header inside background and mask, or just overlay.
            // Simplified: Overlay with top radius same as background.
            radius: 8
            
            // Cover bottom corners to make them square
            Rectangle {
                anchors.bottom: parent.bottom
                height: 8
                width: parent.width
                color: parent.gradient.stops[1].color
            }

            // Content Area
            Loader {
                anchors.centerIn: parent
                source: {
                    if (nodeType === "CameraNode") return "nodes/CameraNode.qml"
                    if (nodeType === "VideoNode") return "nodes/VideoNode.qml"
                    return "" // Fallback (Text below)
                }
            }
            
            // Fallback Title (if no specialized visual or generic node)
            Text {
                text: nodeType
                color: "#aaa"
                font.pixelSize: 12
                anchors.centerIn: parent
                visible: parent.children[0].status !== Loader.Ready // Hide if Loader worked
            }
            
            // Original Title
            Text {
                text: nodeId.substring(0,8)
                color: "#666"
                font.pixelSize: 8
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
            }
        }
        
        // Content Area (Ports placeholder)
        Column {
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 10
            spacing: 5
            
            Repeater {
                model: 2 // Mock inputs
                Row {
                    spacing: 5
                    Rectangle { width: 10; height: 10; radius: 5; color: "#00FF00" } // Port
                    Label { text: "Input " + index; color: "#AAA" }
                }
            }
        }
    }
}
