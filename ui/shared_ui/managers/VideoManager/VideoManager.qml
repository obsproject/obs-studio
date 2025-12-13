import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import NeuralStudio.Blueprint 1.0

BaseManager {
    id: root
    
    title: "Video Assets"
    description: "Browse and add video files to your scene"

    property VideoManagerController controller: null

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // Header actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: controller ? `${controller.availableVideos.length} videos` : "0 videos"
                color: "#CCCCCC"
                font.pixelSize: 12
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Refresh"
                onClicked: {
                    if (controller) {
                        controller.scanVideos()
                    }
                }
            }
        }

        // Videos list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: videosList
                anchors.fill: parent
                model: controller ? controller.availableVideos : []
                spacing: 5

                delegate: Rectangle {
                    width: videosList.width
                    height: 50
                    color: mouseArea.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4
                    border.color: "#444444"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        // Icon
                        Rectangle {
                            width: 32
                            height: 32
                            color: "#FF6B6B"
                            radius: 4
                            
                            Text {
                                anchors.centerIn: parent
                                text: "▶"
                                color: "white"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        // Video info
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

                            RowLayout {
                                spacing: 10
                                
                                Text {
                                    text: modelData.type
                                    color: "#888888"
                                    font.pixelSize: 11
                                }
                                
                                Text {
                                    text: (modelData.size / 1024 / 1024).toFixed(2) + " MB"
                                    color: "#888888"
                                    font.pixelSize: 11
                                }
                            }
                        }

                        // Add button
                        Button {
                            text: "Add"
                            Layout.preferredWidth: 60
                            onClicked: {
                                if (controller) {
                                    controller.createVideoNode(modelData.path)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }

        // Empty state
        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: "No video files found.\\nPlace .mp4, .mov, or .avi files in assets/video"
            color: "#666666"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            visible: controller && controller.availableVideos.length === 0
        }
    }
}
