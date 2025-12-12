import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#222"
    border.color: "#444"
    border.width: 1
    
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "Macros / Hotkeys"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    GridLayout {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 5
        columns: 3
        columnSpacing: 5
        rowSpacing: 5
        
        Repeater {
            model: 9
            delegate: Button {
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "M" + (index + 1)
                palette.button: "#333"
                palette.buttonText: "#fff"
                onClicked: console.log("Macro " + (index + 1) + " triggered")
            }
        }
    }
}
