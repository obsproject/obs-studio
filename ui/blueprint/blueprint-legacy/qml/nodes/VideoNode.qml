import QtQuick
import QtQuick.Controls

Item {
    id: root
    
    property bool isPlaying: false
    
    Column {
        anchors.centerIn: parent
        spacing: 8
        
        // Mock Video Thumbnail / Placeholder
        Rectangle {
            width: 60
            height: 35
            color: "#000"
            border.color: "#444"
            
            Text {
                text: "VIDEO"
                color: "#444"
                font.bold: true
                anchors.centerIn: parent
            }
            
            // Play Icon Overlay
            Text {
                text: root.isPlaying ? "⏸" : "▶"
                color: "white"
                font.pixelSize: 16
                anchors.centerIn: parent
                visible: true 
            }
        }
        
        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            
            Button {
                text: "Play"
                height: 20
                width: 40
                background: Rectangle { color: "#333"; radius: 4 }
                contentItem: Text { text: "▶"; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: root.isPlaying = true
            }
             Button {
                text: "Stop"
                height: 20
                width: 40
                background: Rectangle { color: "#333"; radius: 4 }
                contentItem: Text { text: "⏹"; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                 onClicked: root.isPlaying = false
            }
        }
    }
}
