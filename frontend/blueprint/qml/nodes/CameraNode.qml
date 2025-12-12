import QtQuick
import QtQuick.Controls

Item {
    id: root
    
    // Mock properties
    property bool isActive: true
    property string statusText: "Active"
    property string resolution: "1920x1080"
    
    Column {
        anchors.centerIn: parent
        spacing: 5
        
        // Mock Lens Graphic
        Rectangle {
            width: 40
            height: 40
            radius: 20
            color: "#111"
            border.color: "#666"
            border.width: 2
            anchors.horizontalCenter: parent.horizontalCenter
            
            Rectangle {
                width: 20; height: 20; radius: 10
                color: "#222"
                anchors.centerIn: parent
                
                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: "#00b2ff"
                    opacity: 0.8
                    anchors.centerIn: parent
                }
            }
        }
        
        Text {
            text: root.resolution
            color: "#aaa"
            font.pixelSize: 10
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Rectangle {
            width: 6; height: 6; radius: 3
            color: root.isActive ? "#00ff00" : "#ff0000"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
