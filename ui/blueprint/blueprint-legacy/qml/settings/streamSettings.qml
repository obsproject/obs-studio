import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: 15
        
        GroupBox {
            title: "Stream"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                
                Label { text: "Service:" }
                ComboBox {
                    model: ["Twitch", "YouTube", "Facebook", "Custom"]
                    currentIndex: model.indexOf(settingsViewModel.streamSettings.service)
                    onActivated: settingsViewModel.updateSetting("stream", "service", currentText)
                }
                
                Label { text: "Server:" }
                TextField {
                    text: settingsViewModel.streamSettings.server
                    onEditingFinished: settingsViewModel.updateSetting("stream", "server", text)
                }
                
                Label { text: "Stream Key:" }
                TextField {
                    text: settingsViewModel.streamSettings.key
                    echoMode: TextInput.Password
                    onEditingFinished: settingsViewModel.updateSetting("stream", "key", text)
                }
            }
        }
    }
}
