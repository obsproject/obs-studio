import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: settingsDialog
    width: 800
    height: 600
    title: "Settings"
    modality: Qt.ApplicationModal
    
    color: "#1e1e1e"
    
    RowLayout {
        anchors.fill: parent
        spacing: 0
        
        // Category List
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: "#252525"
            
            ListView {
                id: categoryList
                anchors.fill: parent
                anchors.margins: 5
                
                model: ListModel {
                    ListElement { name: "General"; category: "general" }
                    ListElement { name: "Stream"; category: "stream" }
                    ListElement { name: "Output"; category: "output" }
                    ListElement { name: "Audio"; category: "audio" }
                    ListElement { name: "Video"; category: "video" }
                }
                
                delegate: ItemDelegate {
                    width: ListView.view.width
                    height: 40
                    
                    background: Rectangle {
                        color: ListView.isCurrentItem ? "#3d3d3d" : "transparent"
                    }
                    
                    contentItem: Text {
                        text: model.name
                        color: "#ddd"
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        categoryList.currentIndex = index
                        settingsLoader.source = ""
                        settingsLoader.source = "settings/" + model.category + "Settings.qml"
                    }
                }
                
                Component.onCompleted: currentIndex = 0
            }
        }
        
        // Settings Content
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                Loader {
                    id: settingsLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    source: "settings/generalSettings.qml"
                }
                
                // Apply/Cancel Buttons
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    
                    Item { Layout.fillWidth: true }
                    
                    Button {
                        text: "Reset"
                        onClicked: settingsViewModel.reset()
                    }
                    
                    Button {
                        text: "Apply"
                        onClicked: {
                            settingsViewModel.applyChanges()
                        }
                    }
                    
                    Button {
                        text: "OK"
                        onClicked: {
                            settingsViewModel.applyChanges()
                            settingsDialog.close()
                        }
                    }
                    
                    Button {
                        text: "Cancel"
                        onClicked: settingsDialog.close()
                    }
                }
            }
        }
    }
}
