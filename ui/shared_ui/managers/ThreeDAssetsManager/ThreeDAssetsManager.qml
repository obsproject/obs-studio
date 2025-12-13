import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import NeuralStudio.Blueprint 1.0

// Base Manager provides common panel structure
BaseManager {
    id: root
    
    title: "3D Assets"
    description: "Browse and add 3D models to your scene"

    // Bind to C++ controller
    property ThreeDAssetsManagerController controller: null

    // Content area
    content: ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // Header actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: controller ? `${controller.availableAssets.length} assets` : "0 assets"
                color: "#CCCCCC"
                font.pixelSize: 12
            }

            Item { Layout.fillWidth: true }  // Spacer

            Button {
                text: "Refresh"
                onClicked: {
                    if (controller) {
                        controller.scanAssets()
                    }
                }
            }
        }

        // Assets list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: assetsList
                anchors.fill: parent
                model: controller ? controller.availableAssets : []
                spacing: 5

                delegate: Rectangle {
                    width: assetsList.width
                    height: 40
                    color: mouseArea.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4
                    border.color: "#444444"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        // Icon (placeholder for now)
                        Rectangle {
                            width: 24
                            height: 24
                            color: "#4CAF50"
                            radius: 4
                            
                            Text {
                                anchors.centerIn: parent
                                text: "3D"
                                color: "white"
                                font.pixelSize: 10
                                font.bold: true
                            }
                        }

                        // Asset info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: modelData.name
                                color: "#FFFFFF"
                                font.pixelSize: 14
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: modelData.type
                                color: "#888888"
                                font.pixelSize: 11
                            }
                        }

                        // Add button
                        Button {
                            text: "Add"
                            Layout.preferredWidth: 60
                            onClicked: {
                                if (controller) {
                                    controller.createThreeDModelNode(modelData.path)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton  // Don't consume clicks
                    }
                }
            }
        }

        // Empty state
        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: "No 3D assets found.\nPlace .obj, .glb, or .fbx files in assets/3d"
            color: "#666666"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            visible: controller && controller.availableAssets.length === 0
        }
    }
}
