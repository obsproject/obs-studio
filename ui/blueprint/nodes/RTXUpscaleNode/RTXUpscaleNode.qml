import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../"  // Import BaseNode
import NeuralStudio.UI 1.0

/**
 * RTXUpscaleNode - Visual node for Blueprint editor
 */
BaseNode {
    id: root
    
    // Node properties
    property var controller: internalController
    property string nodeId: ""
    property int qualityMode: 1
    property int scaleFactor: 2
    
    // Controller
    RTXUpscaleNodeController {
        id: internalController
        qualityMode: root.qualityMode
        scaleFactor: root.scaleFactor
        onQualityModeChanged: root.qualityMode = qualityMode
        onScaleFactorChanged: root.scaleFactor = scaleFactor
    }
    
    // BaseNode properties
    title: "RTX AI Upscale"
    headerColor: "#76B900"  // NVIDIA green
    
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
            text: "4K Input"
            color: "white"
            font.pixelSize: 11
        }
    }
    
    // Quality mode
    Row {
        spacing: 8
        Layout.fillWidth: true
        
        Text {
            text: "Quality:"
            color: "#BDC3C7"
            font.pixelSize: 10
        }
        
        ComboBox {
            model: ["Performance", "High Quality"]
            currentIndex: qualityMode
            font.pixelSize: 10
            Layout.fillWidth: true
            
            onCurrentIndexChanged: {
                root.qualityMode = currentIndex
            }
        }
    }
    
    // Scale factor
    Row {
        spacing: 8
        Layout.fillWidth: true
        
        Text {
            text: "Scale:"
            color: "#BDC3C7"
            font.pixelSize: 10
        }
        
        SpinBox {
            from: 1
            to: 4
            value: scaleFactor
            font.pixelSize: 10
            
            onValueChanged: {
                root.scaleFactor = value
            }
        }
        
        Text {
            text: "x"
            color: "#BDC3C7"
            font.pixelSize: 10
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
            text: "8K Output"
            color: "white"
            font.pixelSize: 11
        }
    }
}
