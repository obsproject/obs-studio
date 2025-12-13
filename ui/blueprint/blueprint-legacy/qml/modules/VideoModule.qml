import QtQuick
import QtQuick.Controls
import QtQuick.Multimedia

Rectangle {
    id: root
    color: "black"
    border.color: "#444"
    border.width: 1
    
    // Header
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "Video Source"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    MediaPlayer {
        id: player
        source: "file:///home/subtomic/Videos/test.mp4" // Placeholder or bindable
        audioOutput: AudioOutput {
            id: audioOutput
            volume: volumeSlider.value
        }
    }
    
    VideoOutput {
        anchors.top: header.bottom
        anchors.bottom: controls.top
        anchors.left: parent.left
        anchors.right: parent.right
        fillMode: VideoOutput.PreserveAspectFit
    }
    
    // Playback Controls
    Column {
        id: controls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 5
        spacing: 5
        
        // Seek Bar
        Row {
            width: parent.width
            spacing: 5
            Label { text: formatTime(player.position); color: "#aaa" }
            Slider {
                width: parent.width - 100
                from: 0
                to: player.duration
                value: player.position
                onMoved: player.position = value
            }
            Label { text: formatTime(player.duration); color: "#aaa" }
        }
        
        // Buttons & Volume
        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            
            Button {
                text: player.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"
                onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
            }
            
            Button {
                text: "Stop"
                onClicked: player.stop()
            }
            
            Label { text: "Vol"; anchors.verticalCenter: parent.verticalCenter }
            Slider {
                id: volumeSlider
                from: 0; to: 1.0; value: 1.0
                width: 80
            }
        }
    }
    
    function formatTime(ms) {
        var totalSeconds = Math.floor(ms / 1000);
        var minutes = Math.floor(totalSeconds / 60);
        var seconds = totalSeconds % 60;
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }
}
