import "../audioclip"
import QtQuick

Item {
    id: root
    objectName: audioclipWidget"

    anchors.fill: parent
    Component.onCompleted: {
        audioclip.updateCurrentTrack();
    }

    MouseArea {
        id: swipeArea

        property int pressX: 0

        anchors.fill: parent
        propagateComposedEvents: true
        onPressed: {
            swipeArea.pressX = mouseX;
        }
        onReleased: {
            var delta = mouseX - swipeArea.pressX;
            var minDelta = 30 * px;
            if (delta > minDelta)
                audioclip.previousTrack();
            else if (delta < -minDelta)
                media.nextTrack();
        }
    }

    ListView {
        id: trackImageListView

        property real itemWidth: 110 * px

        anchors.top: parent.top
        anchors.topMargin: 5 * px
        anchors.right: parent.right
        anchors.rightMargin: 10 * px
        width: 110 * px
        height: width
        model: media.model
        clip: true
        orientation: ListView.Horizontal
        spacing: -0.28 * itemWidth
        highlightMoveDuration: 500
        highlightRangeMode: ListView.StrictlyEnforceRange
        onCurrentIndexChanged: {
            audioclip.currentTrack = currentIndex;
            audioclip.updateCurrentTrack();
        }

        Binding {
            target: trackImageListView
            property: "currentIndex"
            value: audioclip.currentTrack
        }

        delegate: Item {
            id: trackListItem

            readonly property bool isSelected: model.index === audioclip.currentTrack
            property real isSelectedAnimated: isSelected

            width: trackImageListView.itemWidth
            height: width
            // Selected item is rendered on top
            z: isSelected

            Image {
                anchors.fill: parent
                anchors.margins: 5 * px
                scale: 0.75 + isSelectedAnimated * 0.15
                opacity: 0.6 + isSelectedAnimated * 0.4
                fillMode: Image.PreserveAspectFit
                source: model.albumUrl != "" ? "../images/albums/" + model.albumUrl : ""

                Image {
                    z: -1
                    anchors.fill: parent
                    anchors.margins: -0.14 * trackListItem.width
                    fillMode: Image.PreserveAspectFit
                    source: "../images/album-shadow.png"
                    opacity: 0.4 + isSelectedAnimated * 0.4
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    audio.currentTrack = model.index;
                    audio.updateCurrentTrack();
                }
            }

            Behavior on isSelectedAnimated {
                NumberAnimation {
                    duration: 400
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

    Item {
        id: trackInfoArea

        anchors.bottom: audioclip.PlaybackItem.top
        anchors.bottomMargin: 10 * px
        width: parent.width - trackImageListView.width - 22 * px
        height: 60 * px
        x: 12 * px

        Text {
            id: trackNameItem

            font.pixelSize: 14 * px
            font.bold: true
            width: parent.width
            elide: Text.ElideRight
            text: media.trackName
            color: dStyle.fontColor1
        }

        Text {
            id: artistNameItem

            y: 20 * px
            font.pixelSize: 12 * px
            width: parent.width
            elide: Text.ElideRight
            text: audioclip.artistName
            color: dStyle.fontColor1
        }

        Text {
            id: albumNameItem

            y: 40 * px
            font.pixelSize: 10 * px
            width: parent.width
            elide: Text.ElideRight
            text: audioclip.albumName
            color: dStyle.fontColor1
            opacity: 0.5
        }
    }

    AudioClipControls {
        id: audioclipPlaybackItem

        width: parent.width
        anchors.bottom: parent.bottom
    }
}
