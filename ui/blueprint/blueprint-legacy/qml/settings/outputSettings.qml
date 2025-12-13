import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: 15
        
        GroupBox {
            title: "Streaming"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                
                Label { text: "Bitrate (kbps):" }
                SpinBox {
                    from: 500
                    to: 50000
                    stepSize: 500
                    value: settingsViewModel.outputSettings.streamingBitrate
                    onValueChanged: settingsViewModel.updateSetting("output", "streamingBitrate", value)
                }
                
                Label { text: "Encoder:" }
                ComboBox {
                    model: ["x264", "NVIDIA NVENC H.264", "AMD AMF H.264", "Apple VT H264"]
                    currentIndex: model.indexOf(settingsViewModel.outputSettings.encoder)
                    onActivated: settingsViewModel.updateSetting("output", "encoder", currentText)
                }
            }
        }
        
        GroupBox {
            title: "Recording"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                
                Label { text: "Recording Path:" }
                TextField {
                    Layout.fillWidth: true
                    text: settingsViewModel.outputSettings.recordingPath
                    onEditingFinished: settingsViewModel.updateSetting("output", "recordingPath", text)
                }
                
                Label { text: "Recording Format:" }
                ComboBox {
                    model: ["mkv", "mp4", "flv", "mov"]
                    currentIndex: model.indexOf(settingsViewModel.outputSettings.recordingFormat)
                    onActivated: settingsViewModel.updateSetting("output", "recordingFormat", currentText)
                }
            }
        }
    }
}
