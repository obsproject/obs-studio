import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    Item {
        id: viewContainer
        anchors.fill: parent
        anchors.rightMargin: 300 // Reserve space for properties
        
        StackLayout {
            id: viewStack
            anchors.fill: parent
            currentIndex: 0
            
            // 2D Node Graph (default)
            NodeGraph {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            
            // 3D Spatial View
            SpatialNodeGraph {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
    
    // Properties Panel (Right)
    PropertiesPanel {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 300
    }
    
    // View Toggle Button
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        spacing: 5
        z: 100
        
        Button {
            text: viewStack.currentIndex === 0 ? "3D View" : "2D View"
            onClicked: viewStack.currentIndex = (viewStack.currentIndex + 1) % 2
        }
    }
    
    Text {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 10
        text: viewStack.currentIndex === 0 ? "Blueprint (2D Graph)" : "Blueprint (3D Spatial)"
        color: "white"
        font.pixelSize: 14
        opacity: 0.7
        z: 100
    }
}
