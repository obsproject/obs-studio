import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#252525"
    border.color: "#444"
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Header
        Text {
            text: "Layer Mixer"
            color: "white"
            font.bold: true
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }
        
        Rectangle { height: 1; color: "#444"; Layout.fillWidth: true }
        
        // Layout selector
        Row {
            Layout.fillWidth: true
            spacing: 5
            
            Label { text: "Layout:"; color: "#ddd"; font.pixelSize: 12 }
            ComboBox {
                model: ["Single", "Picture-in-Picture", "Side-by-Side", "Grid 2x2"]
                
                background: Rectangle {
                    color: "#333"
                    border.color: "#555"
                }
            }
        }
        
        // Video feeds grid
        GroupBox {
            title: "Video Feeds"
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            background: Rectangle {
                color: "#2a2a2a"
                border.color: "#555"
            }
            
            label: Text {
                text: parent.title
                color: "#4a90e2"
                font.bold: true
                font.pixelSize: 12
            }
            
            GridLayout {
                anchors.fill: parent
                columns: 2
                rowSpacing: 5
                columnSpacing: 5
                
                // Mock video feeds
                Repeater {
                    model: 4
                    
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        color: "#444"
                        border.color: index === 0 ? "cyan" : "#666"
                        border.width: index === 0 ? 2 : 1
                        radius: 4
                        
                        // Mock video preview
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 2
                            color: "#222"
                            
                            Text {
                                anchors.centerIn: parent
                                text: "📹"
                                font.pixelSize: 32
                                color: "#666"
                            }
                        }
                        
                        // Feed label
                        Text {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.margins: 4
                            text: "Camera " + (index + 1)
                            color: "white"
                            font.pixelSize: 10
                            font.bold: index === 0
                        }
                        
                        // Active indicator
                        Rectangle {
                            visible: index === 0
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: 4
                            width: 8
                            height: 8
                            radius: 4
                            color: "red"
                        }
                    }
                }
            }
        }
        
        // Audio meters
        GroupBox {
            title: "Audio Sources"
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            
            background: Rectangle {
                color: "#2a2a2a"
                border.color: "#555"
            }
            
            label: Text {
                text: parent.title
                color: "#e24a90"
                font.bold: true
                font.pixelSize: 12
            }
            
            RowLayout {
                anchors.fill: parent
                spacing: 10
                
                // Mock audio sources
                Repeater {
                    model: 3
                    
                    delegate: ColumnLayout {
                        Layout.fillHeight: true
                        spacing: 2
                        
                        // Meter
                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.fillHeight: true
                            color: "#333"
                            border.color: "#555"
                            radius: 2
                            
                            Rectangle {
                                id: levelBar
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.margins: 2
                                height: parent.height * 0.5
                                color: "green"
                                radius: 2
                            }
                        }
                        
                        // Label
                        Text {
                            text: index === 0 ? "Host" : index === 1 ? "Guest" : "Amb"
                            color: "white"
                            font.pixelSize: 8
                            horizontalAlignment: Text.AlignHCenter
                            Layout.preferredWidth: 40
                        }
                        
                        // Mute button
                        Button {
                            text: "🔊"
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 20
                            background: Rectangle {
                                color: "#444"
                                radius: 2
                            }
                        }
                    }
                }
            }
        }
    }
}
