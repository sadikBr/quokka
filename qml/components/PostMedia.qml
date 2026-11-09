import QtQuick

// The big media block of a post (image, gallery or video poster), with spoiler/NSFW covers,
// gallery badge and play button. Emits activated() when tapped (and not covered).
Item {
    id: root
    property int kind
    property string previewUrl
    property string previewFallbackUrl
    property int previewWidth
    property int previewHeight
    property int galleryCount
    property bool spoiler
    property bool nsfw
    property int maxHeight: 520
    property real maxRatio: 1.25
    property color backgroundColor: Theme.surface
    signal activated()

    property bool revealed: false
    readonly property bool covered: (spoiler || nsfw) && !revealed
    // The loaded picture's own shape wins; the server's size hint (or 2:3) is used until then.
    readonly property real ratio: image.naturalRatio > 0 ? image.naturalRatio
                                : previewWidth > 0 && previewHeight > 0 ? previewHeight / previewWidth : 0.66

    implicitHeight: Math.min(maxHeight, Math.round(width * Math.max(0.45, Math.min(maxRatio, ratio))))

    RoundedImage {
        id: image
        // When the height limit kicks in, narrow the box to the picture instead of showing bars.
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: Math.max(parent.width * 0.5, Math.min(parent.width, Math.round(parent.height / root.ratio)))
        decodeWidth: root.width
        source: root.previewUrl
        fallbackSource: root.previewFallbackUrl
        backgroundColor: root.backgroundColor
        fillMode: Image.PreserveAspectCrop
        placeholderColor: Theme.surfaceAlt
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: root.covered ? root.revealed = true : root.activated()
    }

    // Spoiler / NSFW cover
    Rectangle {
        anchors.fill: image
        visible: root.covered
        radius: Theme.radiusMedia
        color: Theme.dark ? "#26262C" : "#E9E9EE"
        Column {
            anchors.centerIn: parent
            spacing: 6
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "eye-off"; size: 26; color: Theme.textMuted }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.nsfw ? qsTr("NSFW · tap to show") : qsTr("Spoiler · tap to reveal")
                color: Theme.textMuted
                font.pixelSize: Theme.fontMeta
                font.weight: Font.Bold
            }
        }
    }

    Chip {
        visible: root.kind === PostListModel.GalleryPost && !root.covered
        anchors.top: image.top
        anchors.right: image.right
        anchors.margins: 10
        iconName: "gallery"
        text: "1 / " + root.galleryCount
        fillColor: "#B3000000"
        textColor: "white"
        border.width: 0
    }

    Rectangle {
        visible: root.kind === PostListModel.VideoPost && !root.covered
        anchors.centerIn: image
        width: 56
        height: 56
        radius: 28
        color: "#E6FFFFFF"
        Icon { anchors.centerIn: parent; anchors.horizontalCenterOffset: 2; name: "play"; size: 24; color: "#141418" }
    }

}
