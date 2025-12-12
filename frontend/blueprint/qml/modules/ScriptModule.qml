import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#1e1e1e"
    border.color: "#444"
    border.width: 1
    
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "Script Editor (Lua/Python)"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    ScrollView {
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 5
        
        TextArea {
            text: "-- Custom Event Handler\nfunction onSceneLoad()\n  print('Scene Loaded')\nend"
            color: "#dcdccc" // Zenburn-ish
            font.family: "Monospace"
            background: null
            selectByMouse: true
        }
    }
    
    Row {
        id: footer
        height: 30
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 5
        spacing: 5
        
        Button { text: "Run"; height: 24 }
        Button { text: "Save"; height: 24 }
    }
}
