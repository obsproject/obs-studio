import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../"  // Import BaseNode
import NeuralStudio.UI 1.0

/**
 * HeadsetOutputNode - Visual node for Blueprint editor
 */
BaseNode {
    id: root
    
    // Node properties
    property var controller: internalController
    property string nodeId: ""
    property string profileId: "quest3"
    
    // Controller
    HeadsetOutputNodeController {
        id: internalController
        // Bindings
        profileId: root.profileId
        onProfileIdChanged: root.profileId = profileId
    }
    
    // BaseNode properties
    title: "🥽 Headset Output"
    headerColor: "#E74C3C"  // Red
    
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
            text: "Video Input"
            color: "white"
            font.pixelSize: 11
        }
    }
    
    // Profile selector
    Row {
        spacing: 8
        Layout.fillWidth: true
        
        Text {
            text: "Profile:"
            color: "#BDC3C7"
            font.pixelSize: 10
        }
        
        ComboBox {
            model: ["Quest 3", "Valve Index", "Vive Pro 2"]
            currentIndex: profileId === "quest3" ? 0 : (profileId === "index" ? 1 : 2)
            font.pixelSize: 10
            Layout.fillWidth: true
            
            onCurrentIndexChanged: {
                switch (currentIndex) {
                    case 0: root.profileId = "quest3"; break;
                    case 1: root.profileId = "index"; break;
                    case 2: root.profileId = "vivepro2"; break;
                }
            }
        }
    }
    
    // Status indicator
    Row {
        spacing: 8
        Layout.fillWidth: true
        
        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: enabled ? "#2ECC71" : "#95A5A6"  // Green if enabled
        }
        
        Text {
            text: enabled ? "Streaming..." : "Disabled"
            color: "#BDC3C7"
            font.pixelSize: 10
        }
    }
}
