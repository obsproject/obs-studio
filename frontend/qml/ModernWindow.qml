import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "OBS Studio - Modern Expressive"

    Material.theme: Material.Dark
    Material.accent: Material.DeepPurple
    Material.primary: Material.DeepPurple

    property bool isStreaming: false
    property bool isRecording: false

    header: ToolBar {
        Material.elevation: 4
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "☰"
                onClicked: drawer.open()
            }

            Label {
                text: "OBS Studio"
                font.pixelSize: 20
                font.bold: true
                elide: Label.ElideRight
                Layout.leftMargin: 10
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                visible: window.isStreaming || window.isRecording
                spacing: 10
                Layout.rightMargin: 10

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: "red"

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        PropertyAnimation { to: 0.2; duration: 800 }
                        PropertyAnimation { to: 1.0; duration: 800 }
                    }
                }

                Label {
                    text: window.isStreaming ? "LIVE" : "REC"
                    color: "red"
                    font.bold: true
                }
            }
        }
    }

    Drawer {
        id: drawer
        width: Math.min(window.width * 0.66, 300)
        height: window.height

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            Label {
                text: "Settings"
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 20
            }

            Repeater {
                model: ["General", "Stream", "Output", "Audio", "Video", "Hotkeys"]
                delegate: ItemDelegate {
                    text: modelData
                    Layout.fillWidth: true
                    font.pixelSize: 16
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    StackView {
        id: stack
        initialItem: mainView
        anchors.fill: parent
    }

    Component {
        id: mainView
        Page {
            background: Rectangle { color: "#121212" }

            RowLayout {
                anchors.fill: parent
                spacing: 20
                anchors.margins: 20

                // Preview Area
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "black"
                    radius: 12
                    border.color: "#333"
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: "Preview"
                        color: "#666"
                        font.pixelSize: 24
                    }
                }

                // Controls Area
                ColumnLayout {
                    Layout.preferredWidth: 300
                    Layout.fillHeight: true
                    spacing: 20

                    // Scenes Card
                    Pane {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Material.elevation: 2
                        Material.background: "#1E1E1E"

                        ColumnLayout {
                            anchors.fill: parent
                            Label { text: "Scenes"; font.bold: true }
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: ["Scene 1", "Gaming", "Just Chatting", "BRB"]
                                delegate: ItemDelegate {
                                    text: modelData
                                    width: parent.width
                                    highlighted: index === 0
                                }
                            }
                        }
                    }

                    // Sources Card
                    Pane {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Material.elevation: 2
                        Material.background: "#1E1E1E"

                        ColumnLayout {
                            anchors.fill: parent
                            Label { text: "Sources"; font.bold: true }
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: ["Game Capture", "Webcam", "Mic/Aux", "Alert Box"]
                                delegate: ItemDelegate {
                                    text: modelData
                                    width: parent.width
                                }
                            }
                        }
                    }

                    // Main Controls
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            Layout.fillWidth: true
                            text: window.isStreaming ? "Stop Streaming" : "Start Streaming"
                            highlighted: window.isStreaming
                            Material.accent: window.isStreaming ? Material.Red : Material.DeepPurple
                            onClicked: window.isStreaming = !window.isStreaming

                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: parent.down ? Qt.darker(parent.Material.accentColor, 1.2) : parent.Material.accentColor
                                radius: 24

                                Behavior on color { ColorAnimation { duration: 200 } }
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: window.isRecording ? "Stop Recording" : "Start Recording"
                            highlighted: window.isRecording
                            Material.accent: window.isRecording ? Material.Red : Material.BlueGrey
                            onClicked: window.isRecording = !window.isRecording

                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: parent.down ? Qt.darker(parent.Material.accentColor, 1.2) : parent.Material.accentColor
                                radius: 24

                                Behavior on color { ColorAnimation { duration: 200 } }
                            }
                        }
                    }
                }
            }
        }
    }
}
