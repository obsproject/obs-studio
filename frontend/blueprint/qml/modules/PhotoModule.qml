import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Rectangle {
    id: root
    color: "#181818"
    border.color: "#444"
    border.width: 1
    
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "Photo Capture"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            active: true
        }
        imageCapture: ImageCapture {
            id: imageCapture
            onImageCaptured: (requestId, preview) => {
                lastImagePreview.source = preview
            }
        }
        videoOutput: viewfinder
    }
    
    // Layout
    ColumnLayout {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0
        
        // Viewfinder
        VideoOutput {
            id: viewfinder
            Layout.fillWidth: true
            Layout.fillHeight: true
            fillMode: VideoOutput.PreserveAspectCrop
        }
        
        // Controls Area
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#222"
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15
                
                // Last Captured Image Preview
                Rectangle {
                    width: 40; height: 40
                    color: "#000"
                    border.color: "#555"
                    Image {
                        id: lastImagePreview
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectFit
                    }
                }
                
                Button {
                    text: "Capture"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    palette.button: "#444"
                    palette.buttonText: "white"
                    onClicked: imageCapture.capture()
                }
                
                Button {
                    text: "File..."
                    onClicked: console.log("Open Gallery")
                }
            }
        }
    }
}
