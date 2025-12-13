import QtQuick
import QtQuick.Shapes

Item {
    id: root
    clip: true // Ensure content doesn't bleed

    property bool is3DMode: false

    // Control to toggle 2D/3D
    Rectangle {
        id: modeToggle
        width: 120; height: 40
        color: "#333333"
        radius: 20
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        z: 999
        border.color: "#666"
        
        Text {
            anchors.centerIn: parent
            text: root.is3DMode ? "Mode: 3D" : "Mode: 2D"
            color: "white"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: root.is3DMode = !root.is3DMode
        }
    }

    // Transform Area (Handles Zoom/Pan)
    Item {
        id: graphArea
        width: 100000 // Large virtual canvas
        height: 100000
        x: -width/2 + root.width/2
        y: -height/2 + root.height/2
        
        // 3D Perspective Transform
        transform: Rotation {
            id: tilt
            origin.x: root.width/2  // Rotate around center of screen, not large canvas
            origin.y: root.height/2
            axis { x: 1; y: 0; z: 0 }
            angle: root.is3DMode ? -30 : 0
            
            Behavior on angle { NumberAnimation { duration: 600; easing.type: Easing.OutBack } }
        }
        
        // Grid Shader/Pattern
        // Using a shader is best for infinite grid, but Repeater + pattern for prototype is fine.
        // Let's use a nice ShaderEffect for the grid if possible, or large Rectangle with patterned image.
        // For simplicity: A strict grid pattern using Shapes or Rectangles.
        
        Rectangle {
            anchors.fill: parent
            color: "#1e1e1e"
            
            // Grid Lines (every 50px)
            Shape {
                anchors.fill: parent
                // Simplified Grid: Use ShaderEffect in future for perf. 
                // For now, simple repeating background image property or just basic rects.
                // Or better: Let's stick to the Repeater approach but centered? 
                // No, Repeater is bad for 100000px.
                // Best practice: ShaderEffect.
            }
        
            // Fallback: Just a dark bg for now, will implement Shader Grid next step.
        }

        // Lookup map for Node instances
        property var nodeLookup: ({})

        // Connection Layer (Behind Nodes)
        Item {
           id: connLayer
           anchors.fill: parent
           
           Repeater {
               model: connectionModel
               delegate: ConnectionLine {
                   property var src: graphArea.nodeLookup[model.sourceNodeId]
                   property var dst: graphArea.nodeLookup[model.targetNodeId]
                   
                   // Dynamic Bindings
                   startX: src ? src.x + src.width : 0
                   startY: src ? src.y + (src.height/2) : 0
                   endX: dst ? dst.x : 0
                   endY: dst ? dst.y + (dst.height/2) : 0
                   
                   visible: src && dst
               }
           }
        }

        // Node Container linked to Model
        Item {
            id: nodeLayer
            anchors.fill: parent
            
            Repeater {
                id: nodeRepeater
                model: nodeGraphModel
                delegate: Node {
                    // Start position relative to graph center
                    nodeX: model.nodeX + 50000 
                    nodeY: model.nodeY + 50000
                    nodeType: model.nodeType
                    
                    // Inject 3D Mode state
                    is3D: root.is3DMode
                    
                    // Register to lookup
                    Component.onCompleted: {
                        graphArea.nodeLookup[nodeId] = this
                        graphArea.nodeLookupChanged() // Notify changes
                    }
                    Component.onDestruction: {
                        delete graphArea.nodeLookup[nodeId]
                         graphArea.nodeLookupChanged()
                    }
                }
            }
        }
    }

    // Navigation Handlers
    PinchHandler {
        target: graphArea
        minimumScale: 0.1
        maximumScale: 5.0
    }
    
    DragHandler {
        target: graphArea
        acceptedButtons: Qt.MiddleButton
    }
    
    // MouseWheel Zoom
    WheelHandler {
        target: graphArea
        property: "scale"
        acceptedDevices: PointerDevice.Mouse
    }
