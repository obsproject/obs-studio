import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: 15
        
        GroupBox {
            title: "Audio"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                
                Label { text: "Sample Rate:" }
                ComboBox {
                    model: ["44.1khz", "48khz", "96khz"]
                    currentIndex: model.indexOf(settingsViewModel.audioSettings.sampleRate)
                    onActivated: settingsViewModel.updateSetting("audio", "sampleRate", currentText)
                }
                
                Label { text: "Channels:" }
                ComboBox {
                    model: ["Mono", "Stereo", "2.1", "5.1", "7.1"]
                    currentIndex: model.indexOf(settingsViewModel.audioSettings.channels)
                    onActivated: settingsViewModel.updateSetting("audio", "channels", currentText)
                }
                
                Label { text: "Microphone:" }
                TextField {
                    text: settingsViewModel.audioSettings.micDevice
                    onEditingFinished: settingsViewModel.updateSetting("audio", "micDevice", text)
                }
            }
        }
    }
}
