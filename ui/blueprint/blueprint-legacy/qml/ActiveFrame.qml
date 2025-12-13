import QtQuick
import QtQuick.Controls

Item {
    id: root
    
    // Background: Darker, distinct from Blueprint
    Rectangle {
        anchors.fill: parent
        color: "#111111"
    }

    // Grid Layout for Tiling (Placeholder for Wayland/Niri style)
    // Tiling Layout (Wayland Style Simulation)
    GridLayout {
        anchors.fill: parent
        anchors.margins: 4
        columns: 4
        rowSpacing: 4
        columnSpacing: 4
        
        // Col 1: Inputs
        CameraModule {
            Layout.column: 0
            Layout.row: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 300
        }
        
        VideoModule {
            Layout.column: 0
            Layout.row: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        // Col 3: Controls & Mixer (now split into col 2 and 3)
        
        MixerModule {
            Layout.column: 2
            Layout.row: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        MacrosModule {
            Layout.column: 3 
            Layout.row: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        ThreeDModule {
            Layout.column: 2
            Layout.row: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        ScriptModule {
            Layout.column: 3
            Layout.row: 1
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        IncomingStreamModule {
            Layout.column: 0
            Layout.row: 3 // New Row
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.preferredHeight: 200
        }
        
        // Adjust Audio and Photo to new flow
        AudioModule {
            Layout.column: 2
            Layout.row: 2
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.preferredHeight: 200
        }
        
        PhotoModule {
            Layout.column: 2
            Layout.row: 3
            Layout.columnSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        // Col 2: Main Preview (Large)
        Rectangle {
            Layout.column: 1
            Layout.row: 0
            Layout.rowSpan: 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 600
            color: "black"
            border.color: "#555"
            border.width: 2
            
            Text {
                text: "PROGRAM OUT"
                color: "red"
                font.bold: true
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
            }
        }
        anchors.centerIn: parent
        text: "ActiveFrame (Live View)\nPress TAB to Switch to Blueprint"
        color: "white"
        font.pixelSize: 24
        opacity: 0.5
    }
}
