import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

/**
 * BaseNode - Common visual template for all Blueprint nodes
 * 
 * Provides:
 * - Standard styling (rounded corners, borders)
 * - Header bar with title and icon
 * - Drag and drop behavior
 * - Selection state
 * - Content area for derived nodes
 */
Rectangle {
    id: root
    
    // Public properties
    property alias title: titleText.text
    property color headerColor: "#3498DB"
    property bool selected: false
    property bool enabled: true
    
    // Content property to hold derived node content
    default property alias content: contentLayout.data
    
    // Signals
    signal node moved(real x, real y)
    signal nodeSelected()
    
    // Visual style
    width: 200
    height: Math.max(100, contentLayout.implicitHeight + header.height + 20)
    color: "#2C3E50"
    border.color: selected ? "white" : (enabled ? headerColor : "#95A5A6")
    border.width: selected ? 3 : 2
    radius: 8
    
    // Header
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        color: root.enabled ? root.headerColor : "#7F8C8D"
        radius: 8
        
        // Flatten bottom corners
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 8
            color: parent.color
        }
        
        Text {
            id: titleText
            anchors.centerIn: parent
            text: "Node"
            color: "white"
            font.bold: true
            font.pixelSize: 14
        }
        
        // Status indicator (enabled/disabled)
        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            width: 8
            height: 8
            radius: 4
            color: root.enabled ? "#2ECC71" : "#E74C3C"
        }
    }
    
    // Content Container
    ColumnLayout {
        id: contentLayout
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 8
    }
    
    // Drag Handler
    MouseArea {
        id: dragArea
        anchors.fill: header
        drag.target: root
        cursorShape: Qt.OpenHandCursor
        
        onPressed: {
            root.z = 100 // Bring to front
            cursorShape = Qt.ClosedHandCursor
            root.nodeSelected()
        }
        
        onReleased: {
            cursorShape = Qt.OpenHandCursor
            root.moved(root.x, root.y)
        }
    }
    
    // Selection Handler (click body)
    MouseArea {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        propagateComposedEvents: true
        
        onPressed: {
            mouse.accepted = false // Pass through to children
            root.nodeSelected()
        }
    }
    // Controller reference for Port Binding
    property var controller: null

    // ========================================================================
    // PORTS (Visual: Left/Right, Audio: Top/Bottom)
    // ========================================================================

    // Visual Input (Left)
    Rectangle {
        id: visualInPort
        width: 12; height: 12; radius: 6
        color: "#3498DB" // Blue
        border.color: "white"
        border.width: 2
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: -6
        visible: controller ? controller.visualInputEnabled : false
    }

    // Visual Output (Right)
    Rectangle {
        id: visualOutPort
        width: 12; height: 12; radius: 6
        color: "#3498DB" // Blue
        border.color: "white"
        border.width: 2
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: -6
        visible: controller ? controller.visualOutputEnabled : false
    }

    // Audio Input (Top)
    Rectangle {
        id: audioInPort
        width: 12; height: 12; radius: 6
        color: "#F1C40F" // Yellow
        border.color: "white"
        border.width: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: -6
        visible: controller ? controller.audioInputEnabled : false
    }

    // Audio Output (Bottom)
    Rectangle {
        id: audioOutPort
        width: 12; height: 12; radius: 6
        color: "#F1C40F" // Yellow
        border.color: "white"
        border.width: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -6
        visible: controller ? controller.audioOutputEnabled : false
    }
}
