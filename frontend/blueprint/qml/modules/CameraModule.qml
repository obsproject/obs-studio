import QtQuick
import QtQuick.Controls
import QtQuick.Multimedia

Rectangle {
    id: root
    color: "#050505"
    border.color: "#444"
    border.width: 1
    
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "Camera Preview"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            active: false
        }
        videoOutput: videoOutput
    }
    
    VideoOutput {
        id: videoOutput
        anchors.top: header.bottom
        anchors.bottom: controls.top
        anchors.left: parent.left
        anchors.right: parent.right
        fillMode: VideoOutput.PreserveAspectFit
        
        // Touch to Focus (Custom Focus Point)
        MouseArea {
            anchors.fill: parent
            onClicked: (mouse) => {
                var point = Qt.point(mouse.x / width, mouse.y / height);
                camera.customFocusPoint = point;
                camera.focusMode = Camera.FocusModeAutoNear; // Trigger autofocus at point
            }
        }
    }
    
    // Camera Controls
    Column {
        id: controls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 5
        spacing: 5
        
        // Top Row: Active & Zoom
        Row {
            spacing: 10
            Button {
                text: camera.active ? "Stop" : "Start"
                height: 30
                onClicked: camera.active = !camera.active
            }
            
            Label { text: "Zoom:"; anchors.verticalCenter: parent.verticalCenter }
            Slider {
                width: 150
                from: camera.minimumZoomFactor
                to: camera.maximumZoomFactor
                value: 1.0
                onMoved: camera.zoomFactor = value
            }
            Label { text: camera.zoomFactor.toFixed(1) + "x"; anchors.verticalCenter: parent.verticalCenter }
        }
    }
}
