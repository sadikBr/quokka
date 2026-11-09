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
    property int maxHeight: 560
    property real maxRatio: 1.25
    property color backgroundColor: Theme.surface
    signal activated()

    property bool revealed: false
    readonly property bool covered: (spoiler || nsfw) && !revealed
    readonly property real ratio: previewWidth > 0 && previewHeight > 0 ? previewHeight / previewWidth
                                : image.naturalRatio > 0 ? image.naturalRatio : 0.66

    implicitHeight: Math.min(maxHeight, Math.round(width * Math.max(0.45, Math.min(maxRatio, ratio))))

    RoundedImage {
        id: image
        anchors.fill: parent
        source: root.previewUrl
        fallbackSource: root.previewFallbackUrl
        backgroundColor: root.backgroundColor
        fillMode: root.ratio > root.maxRatio ? Image.PreserveAspectCrop : Image.PreserveAspectFit
        placeholderColor: Theme.dark ? "#0E0C13" : "#F4E3D2"
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: root.covered ? root.revealed = true : root.activated()
    }

    // Spoiler / NSFW cover
    Rectangle {
        anchors.fill: parent
        visible: root.covered
        radius: Theme.radiusMedia
        color: root.nsfw ? Theme.nsfw : Theme.spoiler
        Column {
            anchors.centerIn: parent
            spacing: 6
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "eye-off"; size: 30; color: "white" }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.nsfw ? qsTr("NSFW, tap to show") : qsTr("Spoiler, tap to reveal")
                color: "white"
                font.pixelSize: Theme.fontMeta
                font.weight: Font.Bold
            }
        }
    }

    Chip {
        visible: root.kind === PostListModel.GalleryPost && !root.covered
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        iconName: "gallery"
        text: "1 / " + root.galleryCount
        fillColor: "#B3000000"
        textColor: "white"
        border.width: 0
    }

    Rectangle {
        visible: root.kind === PostListModel.VideoPost && !root.covered
        anchors.centerIn: parent
        width: 64
        height: 64
        radius: 32
        color: Theme.alpha(Theme.accent, 0.92)
        border.width: 3
        border.color: "white"
        Icon { anchors.centerIn: parent; anchors.horizontalCenterOffset: 3; name: "play"; size: 28; color: "white" }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: root.visible && root.kind === PostListModel.VideoPost
            NumberAnimation { to: 1.08; duration: 700; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
        }
    }
}
