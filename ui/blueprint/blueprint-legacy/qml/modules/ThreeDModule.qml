import QtQuick
import QtQuick.Controls
import QtQuick3D

Rectangle {
    id: root
    color: "#111"
    border.color: "#444"
    border.width: 1
    
    Rectangle {
        id: header
        height: 24
        width: parent.width
        color: "#333"
        Text {
            text: "3D Viewport"
            color: "#ddd"
            font.pixelSize: 10
            anchors.centerIn: parent
        }
    }
    
    View3D {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        
        environment: SceneEnvironment {
            clearColor: "#222"
            backgroundMode: SceneEnvironment.Color
        }
        
        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 300)
            lookAtNode: cube
        }
        
        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -30
        }
        
        Model {
            id: cube
            source: "#Cube"
            materials: DefaultMaterial {
                diffuseColor: "#cb4b16" // Solarized Orange
            }
            
            NumberAnimation on eulerRotation.y {
                from: 0; to: 360; duration: 10000; loops: Animation.Infinite
            }
            NumberAnimation on eulerRotation.x {
                from: 0; to: 360; duration: 15000; loops: Animation.Infinite
            }
        }
    }
}
