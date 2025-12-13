import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: 15
        
        GroupBox {
            title: "General"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                
                Label { text: "Language:" }
                ComboBox {
                    model: ["English", "Spanish", "French", "German"]
                    currentIndex: model.indexOf(settingsViewModel.generalSettings.language)
                    onActivated: settingsViewModel.updateSetting("general", "language", currentText)
                }
                
                Label { text: "Theme:" }
                ComboBox {
                    model: ["Dark", "Light", "System"]
                    currentIndex: model.indexOf(settingsViewModel.generalSettings.theme)
                    onActivated: settingsViewModel.updateSetting("general", "theme", currentText)
                }
            }
        }
    }
}
