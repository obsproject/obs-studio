import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: root
    color: "#000"
    border.color: "#444"
    border.width: 1
    
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "Incoming Stream (SRT/RTMP)"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    MediaPlayer {
        id: player
        audioOutput: AudioOutput {}
        // source set by input field
    }
    
    VideoOutput {
        anchors.top: header.bottom
        anchors.bottom: controls.top
        anchors.left: parent.left
        anchors.right: parent.right
        fillMode: VideoOutput.PreserveAspectFit
    }
    
    ColumnLayout {
        id: controls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 5
        
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: urlField
                placeholderText: "srt://..."
                text: "srt://127.0.0.1:9000"
                Layout.fillWidth: true
                color: "white"
                background: Rectangle { color: "#222"; border.color: "#555" }
            }
            Button {
                text: "Connect"
                onClicked: {
                    player.source = urlField.text
                    player.play()
                }
            }
        }
        
        Text {
            text: "Status: " + player.playbackState
            color: "#888"
            font.pixelSize: 9
        }
    }
}
