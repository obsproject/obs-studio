import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../"  // Import BaseNode
import NeuralStudio.UI 1.0

/**
 * StitchNode - Visual node for Blueprint editor
 */
BaseNode {
    id: root
    
    // Node properties
    property var controller: internalController // Expose to canvas
    property string nodeId: ""
    property string stmapPath: ""
    
    // Controller
    StitchNodeController {
        id: internalController
        // We will bind properties here or via Connections
    }
    
    // BaseNode properties
    title: "STMap Stitch"
    headerColor: "#3498DB"
    
    // Content
    // Input pin
    Row {
        spacing: 8
        Layout.fillWidth: true
        
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: "#E74C3C"
            border.color: "white"
            border.width: 1
        }
        
        Text {
            text: "Video In"
            color: "white"
            font.pixelSize: 11
        }
    }
    
    // STMap path
    Row {
        spacing: 8
        Layout.fillWidth: true
        
        Text {
            text: "STMap:"
            color: "#BDC3C7"
            font.pixelSize: 10
        }
        
        TextInput {
            text: stmapPath
            color: "white"
            font.pixelSize: 10
            readOnly: false
            selectByMouse: true
            Layout.fillWidth: true
            
            onTextChanged: {
                root.stmapPath = text
                if (internalController) internalController.stmapPath = text
            }
        }
    }
    
    // Output pin
    Row {
        layoutDirection: Qt.RightToLeft
        spacing: 8
        Layout.fillWidth: true
        
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: "#2ECC71"
            border.color: "white"
            border.width: 1
        }
        
        Text {
            text: "Stitched Out"
            color: "white"
            font.pixelSize: 11
        }
    }
}
