import QtQuick

// Thumbnails for images and videos linked in a comment or post text. Tap one to open the viewer.
Flow {
    id: root
    property var items: []          // media maps: url, video, animated, resolve, ...
    property string title           // shown in the viewer
    property int maxItems: 4
    property int tileHeight: 150
    property color backgroundColor: Theme.bg
    spacing: 8
    visible: items.length > 0

    function open(index) {
        Nav.openMedia({ title: root.title, media: root.items }, index)
    }

    Repeater {
        model: root.items.slice(0, root.maxItems)
        delegate: Item {
            id: tile
            required property var modelData
            required property int index
            readonly property bool isVideo: modelData.video === true
            readonly property bool more: index === root.maxItems - 1 && root.items.length > root.maxItems
            height: root.tileHeight
            width: isVideo ? Math.round(height * 16 / 9)
                 : thumb.naturalRatio > 0 ? Math.min(root.width, Math.max(height * 0.6, Math.round(height / thumb.naturalRatio)))
                 : height

            RoundedImage {
                id: thumb
                anchors.fill: parent
                visible: !tile.isVideo
                source: tile.isVideo ? "" : tile.modelData.url
                decodeWidth: 480
                radius: Theme.radiusMedia
                backgroundColor: root.backgroundColor
            }
            // Videos: their poster when there is one, else a dark tile saying where they come from.
            VideoThumb {
                anchors.fill: parent
                visible: tile.isVideo
                item: tile.isVideo ? tile.modelData : ({})
                showHost: true
                decodeWidth: 480
                backgroundColor: root.backgroundColor
            }
            Rectangle {
                anchors.fill: parent
                visible: tile.more
                radius: Theme.radiusMedia
                color: "#99000000"
                Text {
                    anchors.centerIn: parent
                    text: "+" + (root.items.length - root.maxItems + 1)
                    color: "white"
                    font.pixelSize: 22
                    font.weight: Font.Bold
                }
            }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: root.open(tile.index) }
        }
    }
}
