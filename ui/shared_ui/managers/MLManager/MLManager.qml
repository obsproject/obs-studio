import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
BaseManager {
    id: root
    title: "ML Models"
    description: "ONNX machine learning models"
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
                            text: `Node: ${modelData}`
                            color: "#4CAF50"; font.pixelSize: 11
                        }
                    }
                }
            }
        }
        
        // Top bar with count, refresh, and presets
        RowLayout {
            Layout.fillWidth: true
            Text { 
                text: controller ? `${controller.availableModels.length} models` : "0" 
                color: "#CCC"; font.pixelSize: 12 
            }
            Item { Layout.fillWidth: true }
            Button { 
                text: "Presets"
                visible: controller && controller.nodePresets.length > 0
                onClicked: presetsMenu.open()
            }
            Button { text: "Refresh"; onClicked: if (controller) controller.scanModels() }
        }
        
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            ListView {
                anchors.fill: parent
                model: controller ? controller.availableModels : []
                spacing: 5
                delegate: Rectangle {
                    width: parent.width; height: 50
                    color: ma.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4; border.color: modelData.inUse ? "#4CAF50" : "#444"; border.width: modelData.inUse ? 2 : 1
                    
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 8; spacing: 10
                        Rectangle { 
                            width: 32; height: 32
                            color: modelData.inUse ? "#4CAF50" : "#00A67E"
                            radius: 4
                            Text { anchors.centerIn: parent; text: modelData.inUse ? "✓" : "🧠"; font.pixelSize: 18 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Text { 
                                text: modelData.name
                                color: modelData.inUse ? "#4CAF50" : "#FFF"
                                font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true 
                            }
                            Text { 
                                text: modelData.inUse ? "IN USE - ONNX" : "ONNX - " + (modelData.size / 1024 / 1024).toFixed(1) + " MB"
                                color: "#888"; font.pixelSize: 11 
                            }
                        }
                        Button { 
                            text: modelData.inUse ? "In Use" : "Add"
                            enabled: !modelData.inUse
                            Layout.preferredWidth: 60
                            onClicked: if (controller) controller.createMLNode(modelData.path)
                        }
                    }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }
            }
        }
    }
    
    // Presets menu
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
