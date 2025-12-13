import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 300
    color: "#252525"
    border.color: "#444"
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        Text {
            text: "Properties"
            color: "white"
            font.bold: true
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
        }
        
        Rectangle { height: 1; color: "#444"; Layout.fillWidth: true }
        
        // Semantic Properties Section
        GroupBox {
            title: "Semantic Properties"
            Layout.fillWidth: true
            
            background: Rectangle {
                color: "#2a2a2a"
                border.color: "#555"
                radius: 4
            }
            
            label: Text {
                text: parent.title
                color: "#4a90e2"
                font.bold: true
                font.pixelSize: 12
            }
            
            ColumnLayout {
                width: parent.width
                spacing: 10
                
                // Semantic Label
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Label:"
                        color: "#ddd"
                        Layout.preferredWidth: 70
                    }
                    TextField {
                        id: semanticLabel
                        placeholderText: "e.g., Office Desk"
                        Layout.fillWidth: true
                        background: Rectangle {
                            color: "#333"
                            border.color: "#555"
                        }
                        color: "white"
                    }
                }
                
                // Category
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Category:"
                        color: "#ddd"
                        Layout.preferredWidth: 70
                    }
                    ComboBox {
                        id: categoryCombo
                        model: ["General", "Furniture", "Equipment", "Person", "Environment", "Media"]
                        Layout.fillWidth: true
                        background: Rectangle {
                            color: "#333"
                            border.color: "#555"
                        }
                    }
                }
                
                // Stereo Mode
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: "Stereo:"
                        color: "#ddd"
                        Layout.preferredWidth: 70
                    }
                    ComboBox {
                        id: stereoMode
                        model: ["Mono", "Stereo SBS", "Stereo TB", "Left Eye", "Right Eye"]
                        Layout.fillWidth: true
                        background: Rectangle {
                            color: "#333"
                            border.color: "#555"
                        }
                    }
                }
                
                // 3D Position
                Label {
                    text: "3D Position"
                    color: "#aaa"
                    font.pixelSize: 11
                }
                
                GridLayout {
                    columns: 2
                    columnSpacing: 5
                    rowSpacing: 5
                    Layout.fillWidth: true
                    
                    Label { text: "X:"; color: "#ddd"; font.pixelSize: 10 }
                    SpinBox {
                        id: posX
                        from: -1000
                        to: 1000
                        value: 0
                        editable: true
                        Layout.fillWidth: true
                    }
                    
                    Label { text: "Y:"; color: "#ddd"; font.pixelSize: 10 }
                    SpinBox {
                        id: posY
                        from: -1000
                        to: 1000
                        value: 0
                        editable: true
                        Layout.fillWidth: true
                    }
                    
                    Label { text: "Z:"; color: "#ddd"; font.pixelSize: 10 }
                    SpinBox {
                        id: posZ
                        from: -1000
                        to: 1000
                        value: 0
                        editable: true
                        Layout.fillWidth: true
                    }
                }
            }
        }
        
        Rectangle { height: 1; color: "#444"; Layout.fillWidth: true }
        
        ListView {
            id: propList
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: propertiesViewModel // Global context property
            
            delegate: ColumnLayout {
                width: ListView.view.width
                spacing: 5
                
                Text {
                    text: model.p_desc ? model.p_desc : model.p_name
                    color: "#ddd"
                    font.pixelSize: 14
                }
                
                // Dynamic Delegate based on Type (Mock logic for now)
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: {
                        if (model.p_type === 1) return boolDelegate // OBS_PROPERTY_BOOL (mock enum val)
                        return textDelegate // Fallback
                    }
                }
                
                Component {
                    id: boolDelegate
                    CheckBox {
                        checked: model.p_value
                        text: model.p_value ? "Enabled" : "Disabled"
                        onToggled: propertiesViewModel.updatePropertyValue(index, checked)
                    }
                }
                
                Component {
                    id: textDelegate
                     TextField {
                        text: model.p_value
                        onEditingFinished: propertiesViewModel.updatePropertyValue(index, text)
                    }
                }
                
                Item { height: 10; width: 1 } // Spacer
            }
        }
    }
}
