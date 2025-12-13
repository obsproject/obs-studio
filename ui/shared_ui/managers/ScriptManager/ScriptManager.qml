import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
BaseManager {
    id: root
    title: "Scripts & Logic"
    description: "Lua and Python automation scripts"
    property var controller: null
    
    content: ColumnLayout {
        anchors.fill: parent; spacing: 10
        
        // Managed Nodes Section
        GroupBox {
            Layout.fillWidth: true
            title: controller ? `Managed Nodes (${controller.managedNodes.length})` : "Managed Nodes (0)"
            visible: controller && controller.managedNodes.length > 0
            
            ScrollView {
                width: parent.width; height: 60
                ListView {
                    model: controller ? controller.managedNodes : []
                    spacing: 3
                    delegate: Rectangle {
                        width: parent.width; height: 25
                        color: "#2A2A2A"; radius: 3
                        Text {
                            anchors.centerIn: parent
                            text: `Script Node: ${modelData}`
                            color: "#FF6B35"; font.pixelSize: 11
                        }
                    }
                }
            }
        }
        
        RowLayout {
            Layout.fillWidth: true
            Text { 
                text: controller ? `${controller.availableScripts.length} scripts` : "0"
                color: "#CCC"; font.pixelSize: 12 
            }
            Item { Layout.fillWidth: true }
            Button { 
                text: "Presets"
                visible: controller && controller.nodePresets.length > 0
                onClicked: presetsMenu.open()
            }
            Button { text: "Refresh"; onClicked: if (controller) controller.scanScripts() }
        }
        
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            ListView {
                anchors.fill: parent
                model: controller ? controller.availableScripts : []
                spacing: 5
                delegate: Rectangle {
                    width: parent.width; height: 50
                    color: ma.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4
                    border.color: modelData.inUse ? "#FF6B35" : "#444"
                    border.width: modelData.inUse ? 2 : 1
                    
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 8; spacing: 10
                        Rectangle {
                            width: 32; height: 32
                            color: modelData.inUse ? "#FF6B35" : (modelData.language === "lua" ? "#00007C" : "#FFD43B")
                            radius: 4
                            Text {
                                anchors.centerIn: parent
                                text: modelData.inUse ? "✓" : (modelData.language === "lua" ? "🌙" : "🐍")
                                font.pixelSize: 18
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Text { 
                                text: modelData.name
                                color: modelData.inUse ? "#FF6B35" : "#FFF"
                                font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true 
                            }
                            Text { 
                                text: modelData.inUse ? "IN USE - " + modelData.language.toUpperCase() : modelData.language.toUpperCase()
                                color: "#888"; font.pixelSize: 11 
                            }
                        }
                        Button { 
                            text: modelData.inUse ? "In Use" : "Add"
                            enabled: !modelData.inUse
                            Layout.preferredWidth: 60
                            onClicked: if (controller) controller.createScriptNode(modelData.path, modelData.language)
                        }
                    }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }
            }
        }
    }
    
    Menu {
        id: presetsMenu
        title: "Load Preset"
        Repeater {
            model: controller ? controller.nodePresets : []
            MenuItem {
                text: modelData.name
                onTriggered: if (controller) controller.loadPreset(modelData.name, 0, 0)
            }
        }
    }
}
