import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
BaseManager {
    id: root
    title: "Visual Effects"
    description: "Effect presets and configurations"
    property var controller: null
    content: ColumnLayout {
        anchors.fill: parent; spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Text { text: controller ? `${controller.availableEffects.length} effects` : "0"; color: "#CCC"; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Button { text: "Refresh"; onClicked: if (controller) controller.scanEffects() }
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            ListView {
                anchors.fill: parent
                model: controller ? controller.availableEffects : []
                spacing: 5
                delegate: Rectangle {
                    width: parent.width; height: 50
                    color: ma.containsMouse ? "#2A2A2A" : "#1E1E1E"
                    radius: 4; border.color: "#444"; border.width: 1
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 8; spacing: 10
                        Rectangle { width: 32; height: 32; color: "#10B981"; radius: 4
                            Text { anchors.centerIn: parent; text: "★"; color: "white"; font.pixelSize: 18 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Text { text: modelData.name; color: "#FFF"; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                            Text { text: modelData.type; color: "#888"; font.pixelSize: 11 }
                        }
                        Button { text: "Add"; Layout.preferredWidth: 60; onClicked: if (controller) controller.createEffectNode(modelData.path) }
                    }
                    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }
            }
        }
    }
}
