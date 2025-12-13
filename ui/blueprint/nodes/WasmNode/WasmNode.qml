import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../" // Import BaseNode

BaseNode {
    id: root
    title: "WASM Engine"
    headerColor: "#8E44AD" // Purple for WASM/Plugins

    property var controller: null // Bound to WasmNodeController

    // Content specific to WasmNode
    ColumnLayout {
        spacing: 5
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: "WASM Module:"
            color: "#bdc3c7"
            font.pixelSize: 10
        }

        TextField {
            id: pathField
            Layout.fillWidth: true
            placeholderText: "/path/to/module.wasm"
            text: controller ? controller.wasmPath : ""
            onEditingFinished: if (controller) controller.wasmPath = text
        }

        Label {
            text: "Function:"
            color: "#bdc3c7"
            font.pixelSize: 10
        }

        TextField {
            id: funcField
            Layout.fillWidth: true
            placeholderText: "entry_point"
            text: controller ? controller.entryFunction : "process"
            onEditingFinished: if (controller) controller.entryFunction = text
        }
    }
}
