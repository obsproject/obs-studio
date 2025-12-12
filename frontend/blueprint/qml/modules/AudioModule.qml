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
            text: "Audio Manager"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    MediaDevices {
        id: devices
    }
    
    // Audio Monitor Session (Loopback for listening to mic)
    CaptureSession {
        id: audioSession
        audioInput: AudioInput {
            id: audioInput
            muted: inputMute.checked
            volume: inputVol.value
        }
        audioOutput: AudioOutput {
            id: audioOutput
            muted: outputMute.checked
            volume: outputVol.value
        }
    }
    
    ColumnLayout {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 15
        
        // --- INPUT SECTION ---
        GroupBox {
            title: "Audio Input (Mic)"
            Layout.fillWidth: true
            
            ColumnLayout {
                width: parent.width
                
                RowLayout {
                    Label { text: "Device:" }
                    ComboBox {
                        Layout.fillWidth: true
                        model: devices.audioInputs
                        textRole: "description"
                        onActivated: audioInput.device = devices.audioInputs[index]
                    }
                }
                
                RowLayout {
                    Label { text: "Vol:" }
                    Slider {
                        id: inputVol
                        Layout.fillWidth: true
                        from: 0; to: 1.0; value: 1.0
                    }
                    Label { text: Math.round(inputVol.value * 100) + "%" }
                }
                
                CheckBox {
                    id: inputMute
                    text: "Mute Input"
                }
            }
        }
        
        // --- OUTPUT SECTION ---
        GroupBox {
            title: "Audio Output (Monitor)"
            Layout.fillWidth: true
            
            ColumnLayout {
                width: parent.width
                
                RowLayout {
                    Label { text: "Device:" }
                    ComboBox {
                        Layout.fillWidth: true
                        model: devices.audioOutputs
                        textRole: "description"
                        onActivated: audioOutput.device = devices.audioOutputs[index]
                    }
                }
                
                RowLayout {
                    Label { text: "Vol:" }
                    Slider {
                        id: outputVol
                        Layout.fillWidth: true
                        from: 0; to: 1.0; value: 0.5 // Default to 50% to avoid feedback loop
                    }
                    Label { text: Math.round(outputVol.value * 100) + "%" }
                }
                
                CheckBox {
                    id: outputMute
                    text: "Mute Output"
                }
            }
        }
        
        Item { Layout.fillHeight: true } // Spacer
    }
}
