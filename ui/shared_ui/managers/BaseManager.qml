import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

/**
 * BaseManager - Common visual template for Blueprint Manager Panels
 * 
 * Usage:
 * BaseManager {
 *     title: "Camera Manager"
 *     // Content goes here
 * }
 */
Rectangle {
    id: root
    
    // Properties
    property string title: "Manager"
    property color headerColor: "#27ae60" // Default green for managers
    
    // Content property
    default property alias content: contentLayout.data
    
    // Style
    width: 300
    height: 400
    color: "#2C3E50"
    radius: 8
    border.color: "#34495E"
    border.width: 1
    
    // Header
    Rectangle {
        id: header
        height: 40
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        
        color: root.headerColor
        radius: 8
        
        // Flatten bottom
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 10
            color: parent.color
        }
        
        Label {
            anchors.centerIn: parent
            text: root.title
            color: "white"
            font.bold: true
            font.pixelSize: 16
        }
    }
    
    // Content Area
    ColumnLayout {
        id: contentLayout
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        spacing: 10
    }
}
