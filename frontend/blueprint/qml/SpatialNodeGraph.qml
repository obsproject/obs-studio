import QtQuick
import QtQuick.Controls
import QtQuick3D

Rectangle {
    id: root
    color: "#1e1e1e"
    
    View3D {
        id: view3d
        anchors.fill: parent
        
        environment: SceneEnvironment {
            clearColor: "#1e1e1e"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }
        
        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 5, 15)
            eulerRotation.x: -15
            
            // Orbit controls
            property real orbitDistance: 15
            property real orbitAngleX: -15
            property real orbitAngleY: 0
        }
        
        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -50
            brightness: 1.5
        }
        
        // Grid Floor
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(20, 20, 1)
            eulerRotation.x: -90
            materials: DefaultMaterial {
                diffuseColor: "#333"
                opacity: 0.3
            }
        }
        
        // Nodes from NodeModel
        Repeater3D {
            model: nodeGraphModel
            
            delegate: Node3D {
                id: delegateNode
                position: Qt.vector3d(model.x || 0, model.y || 0, model.z || 0)
                
                Model {
                    id: nodeVisual
                    source: model.nodeType === "CameraNode" ? "#Sphere" : "#Cube"
                    scale: Qt.vector3d(0.5, 0.5, 0.5)
                    
                    materials: DefaultMaterial {
                        diffuseColor: model.is3D ? "#4a90e2" : "#e24a4a"
                        specularAmount: 0.5
                    }
                    
                    // Selection highlight
                    pickable: true
                }
                
                // Label (Billboard Text)
                Node {
                    position: Qt.vector3d(0, 1, 0)
                    
                    Model {
                        source: "#Rectangle"
                        scale: Qt.vector3d(0.05, 0.02, 1)
                        materials: DefaultMaterial {
                            diffuseMap: Texture {
                                sourceItem: Text {
                                    text: model.label || model.uuid
                                    color: "white"
                                    font.pixelSize: 64
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                        eulerRotation.y: camera.eulerRotation.y
                    }
                }
            }
        }
    }
    
    // Camera Controls
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.MiddleButton | Qt.LeftButton
        
        property point lastPos
        
        onPressed: (mouse) => {
            lastPos = Qt.point(mouse.x, mouse.y)
        }
        
        onPositionChanged: (mouse) => {
            if (pressed && mouse.buttons & Qt.MiddleButton) {
                // Orbit
                let dx = mouse.x - lastPos.x
                let dy = mouse.y - lastPos.y
                
                camera.orbitAngleY += dx * 0.5
                camera.orbitAngleX += dy * 0.5
                camera.orbitAngleX = Math.max(-89, Math.min(89, camera.orbitAngleX))
                
                updateCameraPosition()
                lastPos = Qt.point(mouse.x, mouse.y)
            }
        }
        
        onWheel: (wheel) => {
            camera.orbitDistance -= wheel.angleDelta.y * 0.01
            camera.orbitDistance = Math.max(3, Math.min(50, camera.orbitDistance))
            updateCameraPosition()
        }
    }
    
    function updateCameraPosition() {
        let angleXRad = camera.orbitAngleX * Math.PI / 180
        let angleYRad = camera.orbitAngleY * Math.PI / 180
        
        camera.position.x = camera.orbitDistance * Math.cos(angleXRad) * Math.sin(angleYRad)
        camera.position.y = camera.orbitDistance * Math.sin(angleXRad)
        camera.position.z = camera.orbitDistance * Math.cos(angleXRad) * Math.cos(angleYRad)
        
        camera.eulerRotation.x = -camera.orbitAngleX
        camera.eulerRotation.y = camera.orbitAngleY
    }
    
    // Toggle Button (2D/3D View)
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 5
        
        Button {
            text: "Reset Camera"
            onClicked: {
                camera.orbitAngleX = -15
                camera.orbitAngleY = 0
                camera.orbitDistance = 15
                updateCameraPosition()
            }
        }
    }
}
