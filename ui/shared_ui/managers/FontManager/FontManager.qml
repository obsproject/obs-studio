import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

BaseManager {
    id: root
    title: "Font Assets"
    description: "Typography and text rendering fonts"
    property var controller: null

    content: ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: controller ? `${controller.availableFonts.length} fonts` : "0 fonts"
                color: "#CCCCCC"
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Refresh"
                onClicked: if (controller) controller.scanFonts()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: fontsList
                anchors.fill: parent
                model: controller ? controller.availableFonts : []
                spacing: 5

                delegate: Rectangle {
                    width: fontsList.width
                    height: 50
                    color: mouseArea.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4
                    border.color: "#444444"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        Rectangle {
                            width: 32
                            height: 32
                            color: "#A78BFA"
                            radius: 4
                            Text {
                                anchors.centerIn: parent
                                text: "Aa"
                                color: "white"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

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
                                    text: (modelData.size / 1024).toFixed(1) + " KB"
                                    color: "#888888"
                                    font.pixelSize: 11
                                }
                            }
                        }

                        Button {
                            text: "Add"
                            Layout.preferredWidth: 60
                            onClicked: if (controller) controller.createFontNode(modelData.path)
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

        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: "No fonts found.\\nPlace .ttf, .otf, or .woff files in assets/fonts"
            color: "#666666"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            visible: controller && controller.availableFonts.length === 0
        }
    }
}
