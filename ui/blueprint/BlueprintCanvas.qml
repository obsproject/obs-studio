import QtQuick
import QtQuick.Controls
import QtQuick.Shapes 1.15
import NeuralStudio.UI 1.0 // Assumes NodeGraphController is registered here

Item {
    id: root
    anchors.fill: parent
    clip: true

    property int gridStep: 50
    property color gridColor: "#2A2A2A"
    property color backgroundColor: "#1e1e1e"

    // Toolbar
    Row {
        z: 100 // Top of everything
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        spacing: 10
        
        Button {
            text: "Save Graph"
            onClicked: {
                if (root.graphController.saveGraph("/tmp/graph.json")) {
                    console.log("Graph saved to /tmp/graph.json")
                } else {
                    console.error("Failed to save graph")
                }
            }
        }
        
        Button {
            text: "Load Graph"
            onClicked: {
                if (root.graphController.loadGraph("/tmp/graph.json")) {
                    console.log("Graph loaded from /tmp/graph.json")
                } else {
                    console.error("Failed to load graph")
                }
            }
        }

        Button {
            text: "Clear"
            onClicked: {
                // Should implement clear in controller, for now just load empty or relying on logic
            }
        }
    }

    // Controller Access
    // In a real app, this might be a singleton or context property.
    // For now, let's assume we can instantiate it or it's passed in.
    // effectiveGraphController is either an injected property or a local instance
    property NodeGraphController graphController: NodeGraphController {
        id: internalController
        onNodeCreated: (nodeId, nodeType, x, y, z) => {
            console.log("Node created:", nodeId, nodeType)
            nodeModel.append({ "nodeId": nodeId, "nodeType": nodeType, "x": x, "y": y, "z": z })
        }
        onNodeDeleted: (nodeId) => {
            for(let i=0; i<nodeModel.count; ++i) {
                if(nodeModel.get(i).nodeId === nodeId) {
                    nodeModel.remove(i);
                    break;
                }
            }
        }
        onConnectionCreated: (srcNode, srcPin, dstNode, dstPin) => {
             connectionModel.append({ 
                 "sourceNodeId": srcNode, "sourcePinId": srcPin,
                 "targetNodeId": dstNode, "targetPinId": dstPin 
             })
        }
    }

    // Models
    ListModel { id: nodeModel }
    ListModel { id: connectionModel }

    // Canvas Area
    Rectangle {
        id: canvas
        color: root.backgroundColor
        width: 100000 
        height: 100000
        x: -width/2 + root.width/2
        y: -height/2 + root.height/2
        
        // Grid Pattern
        Shape {
            anchors.fill: parent
            // Performance warning: This is too heavy for 100k px.
            // Using a simple grid image pattern or shader is better.
            // For prototype, we'll just have a solid background.
        }

        MouseArea {
            id: canvasMouse
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton
            property point lastPos
            
            onPressed: (mouse) => {
                lastPos = Qt.point(mouse.x, mouse.y)
            }
            
            onPositionChanged: (mouse) => {
                if (mouse.buttons & Qt.MiddleButton) {
                    canvas.x += mouse.x - lastPos.x
                    canvas.y += mouse.y - lastPos.y
                }
            }

            onClicked: (mouse) => {
                // Test: Create a node on click (if no node was clicked)
                // In reality, this would be drag-drop from a palette
                if (mouse.button === Qt.LeftButton && (mouse.modifiers & Qt.ControlModifier)) {
                    root.graphController.createNode("StitchNode", mouse.x, mouse.y)
                }
            }
        }

        // Connections Layer
        Item {
            id: connectionsLayer
            anchors.fill: parent
            
            Repeater {
                model: connectionModel
                delegate: Shape {
                    // Logic to find positions of nodes...
                    // This requires a lookup of node instances.
                    // For now, placeholder.
                }
            }
        }

        // Nodes Layer
        Item {
            id: nodesLayer
            anchors.fill: parent
            
            Repeater {
                model: nodeModel
                delegate: Loader {
                    x: model.x
                    y: model.y
                    
                    // Logic to choose QML file based on nodeType
                    // e.g. "StitchNode" -> "../nodes/StitchNode/StitchNode.qml"
                    source: {
                        if (model.nodeType === "StitchNode") return "../nodes/StitchNode/StitchNode.qml"
                        if (model.nodeType === "HeadsetOutputNode") return "../nodes/HeadsetOutputNode/HeadsetOutputNode.qml"
                        if (model.nodeType === "RTXUpscaleNode") return "../nodes/RTXUpscaleNode/RTXUpscaleNode.qml"
                        return "../nodes/BaseNode.qml" // Fallback
                    }
                    
                    onLoaded: {
                        if (item) {
                            item.nodeId = model.nodeId
                            // Link controller if possible
                            if (item.controller) {
                                item.controller.setNodeId(model.nodeId)
                                item.controller.setGraphController(root.graphController)
                            }
                        }
                    }
                    
                    // selection/drag logic...
                }
            }
        }
    }
}
