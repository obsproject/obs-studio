import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform // Native Menus

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 720
    title: qsTr("Neural Studio")
    color: "#1e1e1e"

    Platform.MenuBar {
        Platform.Menu {
            title: qsTr("&File")
            Platform.MenuItem { text: qsTr("&New") }
            Platform.MenuItem { text: qsTr("&Open") }
            Platform.MenuItem { text: qsTr("&Save") }
            Platform.MenuSeparator { }
            Platform.MenuItem { 
                text: qsTr("&Settings")
                onTriggered: settingsDialogLoader.active = true
            }
            Platform.MenuSeparator { }
            Platform.MenuItem { text: qsTr("&Exit"); onTriggered: Qt.quit() }
        }
        Platform.Menu {
            title: qsTr("&View")
            Platform.MenuItem { 
                text: qsTr("Switch View")
                shortcut: "Tab"
                onTriggered: stackLayout.currentIndex = (stackLayout.currentIndex + 1) % 2
            }
        }
    }
    
    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0
        
        BlueprintFrame {}
        ActiveFrame {}
    }
    
    Loader {
        id: settingsDialogLoader
        active: false
        sourceComponent: Component {
            SettingsDialog {
                onClosing: settingsDialogLoader.active = false
            }
        }
        onLoaded: item.show()
    }
}
