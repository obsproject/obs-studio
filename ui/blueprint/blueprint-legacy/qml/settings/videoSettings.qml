import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    clip: true
    
    ColumnLayout {
        width: parent.width
        spacing: 15
        
        GroupBox {
            title: "Video"
            Layout.fillWidth: true
            
            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 10
                
                Label { text: "Base Resolution:" }
                TextField {
                    text: settingsViewModel.videoSettings.baseRenderingResolution
                    onEditingFinished: settingsViewModel.updateSetting("video", "baseRenderingResolution", text)
                }
                
                Label { text: "Output Resolution:" }
                TextField {
                    text: settingsViewModel.videoSettings.outputScaledResolution
                    onEditingFinished: settingsViewModel.updateSetting("video", "outputScaledResolution", text)
                }
                
                Label { text: "FPS:" }
                SpinBox {
                    from: 24
                    to: 144
                    value: settingsViewModel.videoSettings.fpsCommon
                    onValueChanged: settingsViewModel.updateSetting("video", "fpsCommon", value)
                }
                
                Label { text: "Downscale Filter:" }
                ComboBox {
                    model: ["Bilinear", "Bicubic", "Lanczos"]
                    currentIndex: model.indexOf(settingsViewModel.videoSettings.scaleFilter)
                    onActivated: settingsViewModel.updateSetting("video", "scaleFilter", currentText)
                }
            }
        }
    }
}
