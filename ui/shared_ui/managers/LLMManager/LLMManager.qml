import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
BaseManager {
    id: root
    title: "LLM Services"
    description: "Cloud AI language models"
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
                            text: `LLM Node: ${modelData}`
                            color: "#4285F4"; font.pixelSize: 11
                        }
                    }
                }
            }
        }

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
            Button { text: "Refresh"; onClicked: if (controller) controller.refreshModels() }
        }

        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            ListView {
                anchors.fill: parent
                model: controller ? controller.availableModels : []
                spacing: 5
                delegate: Rectangle {
                    property string providerColor: {
                        var p = modelData.provider.toLowerCase();
                        if (p.includes("gemini")) return "#4285F4";
                        if (p.includes("openai") || p.includes("gpt")) return "#10A37F";
                        if (p.includes("claude")) return "#D97757";
                        return "#9B59B6";
                    }

                    width: parent.width; height: 60
                    color: ma.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4
                    border.color: modelData.inUse ? providerColor : "#444"
                    border.width: modelData.inUse ? 2 : 1

                    RowLayout {
                        anchors.fill: parent; anchors.margins: 8; spacing: 10
                        Rectangle {
                            width: 38; height: 38
                            color: modelData.inUse ? providerColor : providerColor
                            opacity: modelData.inUse ? 1.0 : 0.7
                            radius: 6
                            Text {
                                anchors.centerIn: parent
                                text: {
                                    if (modelData.includes("claude")) return "Anthropic"
                                    return "Language Model"
                                }
                                color: "#888"; font.pixelSize: 11
                            }
                        }
                        Button { text: "Add"; Layout.preferredWidth: 60; onClicked: if (controller) controller.createLLMNode(modelData) }
                    }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }
            }
        }
    }
}
