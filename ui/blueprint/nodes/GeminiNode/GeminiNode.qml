import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../" // Import BaseNode

BaseNode {
    id: root
    title: "Gemini AI"
    headerColor: "#8E44AD" // Purple/AI

    property var controller: null // Bound to GeminiNodeController

    ColumnLayout {
        spacing: 5
        anchors.fill: parent
        anchors.margins: 10

        Label {
            text: "API Key:"
            color: "#bdc3c7"
            font.pixelSize: 10
        }

        TextField {
            Layout.fillWidth: true
            placeholderText: "AIzaSy..."
            echoMode: TextInput.Password
            text: controller ? controller.apiKey : ""
            onEditingFinished: if (controller) controller.apiKey = text
        }

        Label {
            text: "Model:"
            color: "#bdc3c7"
            font.pixelSize: 10
        }

        ComboBox {
            Layout.fillWidth: true
            model: ["gemini-1.5-flash", "gemini-1.5-pro", "gemini-1.0-pro"]
            currentIndex: model.indexOf(controller ? controller.modelName : "gemini-1.5-flash")
            onActivated: if (controller) controller.modelName = currentText
        }
    }
}
