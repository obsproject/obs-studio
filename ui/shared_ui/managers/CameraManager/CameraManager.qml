import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import NeuralStudio.UI 1.0
import "../" // For BaseManager.qml

BaseManager {
    id: root
    title: "Camera Manager"
    headerColor: "#E74C3C"

    // Instantiate Controller
    CameraManagerController {
        id: controller
        
        // Auto-refresh when opened/created
        Component.onCompleted: scanCameras()
        
        // Bind graphController
        graphController: root.parent && root.parent.graphController ? root.parent.graphController : null
    }

    // Content
    ColumnLayout {
        anchors.fill: parent
        spacing: 5

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: "Available Devices"
                color: "white"
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: "Refresh"
                flat: true
                onClicked: controller.scanCameras()
                background: Rectangle { color: "transparent" }
                contentItem: Text { 
                    text: parent.text; color: "#3498DB"; font.bold: true 
                }
            }
        }

        // Camera List
        ListView {
            id: cameraList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: controller.availableCameras
            spacing: 5

            delegate: Rectangle {
                width: ListView.view.width
                height: 40
                color: "#34495E"
                radius: 4

                Label {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 10
                    text: modelData
                    color: "white"
                    elide: Text.ElideMiddle
                }

                Button {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 5
                    text: "Add"
                    height: 30
                    
                    onClicked: {
                        controller.createCameraNode(modelData)
                    }
                }
            }
        
            // Empty State
            Label {
                visible: cameraList.count === 0
                anchors.centerIn: parent
                text: "No cameras found.\nCheck connections or permissions."
                color: "#95A5A6"
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
