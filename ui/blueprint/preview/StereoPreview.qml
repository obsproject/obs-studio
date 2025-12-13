import QtQuick 2.15
import QtQuick.Controls 2.15
import "."

/**
 * StereoPreview - 3D Stereoscopic Preview (SBS/TB)
 */
BasePreview {
    id: root
    title: "Stereo 3D Preview"
    
    // Stereo Mode Enum
    enum Mode {
        SBS,
        TopBottom,
        LeftOnly,
        RightOnly
    }
    
    property int stereoMode: StereoPreview.Mode.SBS

    // Control Overlay
    Rectangle {
        color: "#AA000000"
        height: 40
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        
        Row {
            anchors.centerIn: parent
            spacing: 10
            
            RadioButton {
                text: "SBS"
                checked: root.stereoMode === StereoPreview.Mode.SBS
                onClicked: root.stereoMode = StereoPreview.Mode.SBS
            }
            RadioButton {
                text: "L"
                checked: root.stereoMode === StereoPreview.Mode.LeftOnly
                onClicked: root.stereoMode = StereoPreview.Mode.LeftOnly
            }
            RadioButton {
                text: "R"
                checked: root.stereoMode === StereoPreview.Mode.RightOnly
                onClicked: root.stereoMode = StereoPreview.Mode.RightOnly
            }
        }
    }
    
    // Split Line Indicator (Visible in SBS mode)
    Rectangle {
        anchors.centerIn: parent
        width: 2
        height: parent.height
        color: "#00FF00"
        opacity: 0.5
        visible: root.stereoMode === StereoPreview.Mode.SBS
    }
}
