import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../" // Import BaseNode

BaseNode {
    id: root
    title: "ONNX Runtime"
    headerColor: "#2980B9" // Blue for AI/Logic

    property var controller: null // Bound to OnnxNodeController

    ColumnLayout {
        spacing: 5
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: "Model Path:"
            color: "#bdc3c7"
            font.pixelSize: 10
        }

        TextField {
            Layout.fillWidth: true
            placeholderText: "/path/to/model.onnx"
            text: controller ? controller.modelPath : ""
            onEditingFinished: if (controller) controller.modelPath = text
        }

        CheckBox {
            text: "Enable GPU Acceleration"
            checked: controller ? controller.useGpu : false
            onCheckedChanged: if (controller) controller.useGpu = checked
            
            contentItem: Text {
                text: parent.text
                color: "#bdc3c7"
                font: parent.font
                leftPadding: parent.indicator.width + parent.spacing
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
