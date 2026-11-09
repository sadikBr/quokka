import QtQuick
import QtMultimedia

// The big media block of a post: picture, video poster (playing muted in place when `play` is
// set), or a grid for galleries. Spoiler/NSFW covers, play button. Emits activated(index) when
// tapped (and not covered), with the index of the gallery item that was tapped.
Item {
    id: root
    property int kind
    property string previewUrl
    property string previewFallbackUrl
    property int previewWidth
    property int previewHeight
    property int galleryCount
    property var media: []           // the post's media maps (url, video, resolve, fallbackUrl, ...)
    property bool spoiler
    property bool nsfw
    property bool play: false        // play a video / GIF in place (muted, looping)
    property int maxHeight: 520
    property real maxRatio: 1.25
    property color backgroundColor: Theme.surface
    signal activated(int index)

    property bool revealed: false
    readonly property bool covered: (spoiler || (nsfw && AppSettings.coverNsfw)) && !revealed
    readonly property bool grid: kind === PostListModel.GalleryPost && media.length > 1
    readonly property var firstVideo: {
        for (let i = 0; i < media.length; ++i)
            if (media[i].video) return media[i]
        return null
    }
    readonly property bool gif: /\.gif(\?|$)/i.test(previewUrl)
    readonly property bool inlineVideo: play && !covered && !grid && kind === PostListModel.VideoPost && firstVideo !== null
    readonly property bool inlineGif: play && !covered && !grid && kind === PostListModel.ImagePost && gif

    // The loaded picture's own shape wins; the server's size hint (or 2:3) is used until then.
    readonly property real ratio: grid ? 0.66
                                : image.naturalRatio > 0 ? image.naturalRatio
                                : previewWidth > 0 && previewHeight > 0 ? previewHeight / previewWidth
                                : previewUrl === "" ? 0.5625 : 0.66   // 16:9 for a video without a picture

    implicitHeight: Math.min(maxHeight, Math.round(width * Math.max(0.45, Math.min(maxRatio, ratio))))

    // A card-sized copy of a gallery picture (Lemmy's image server resizes on request).
    function thumbFor(url) {
        url = String(url)
        if (url.indexOf("/pictrs/image/") >= 0 && url.indexOf("?") < 0 && !/\.gif$/i.test(url))
            return url + "?thumbnail=640&format=jpg"
        return url
    }

    RoundedImage {
        id: image
        visible: !root.grid
        // When the height limit kicks in, narrow the box to the picture instead of showing bars.
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: root.previewUrl === "" ? parent.width
             : Math.max(parent.width * 0.5, Math.min(parent.width, Math.round(parent.height / root.ratio)))
        decodeWidth: root.width
        source: root.grid ? "" : root.previewUrl
        fallbackSource: root.grid ? "" : root.previewFallbackUrl
        backgroundColor: root.backgroundColor
        fillMode: Image.PreserveAspectCrop
        placeholderColor: Theme.surfaceAlt
    }

    // A video post without a picture of its own: a looked-up poster (redgifs) or a dark tile.
    VideoThumb {
        anchors.fill: image
        visible: !root.grid && root.kind === PostListModel.VideoPost && root.previewUrl === ""
        item: visible && root.firstVideo ? root.firstVideo : ({})
        showPlay: false
        backgroundColor: root.backgroundColor
    }

    // ---- Playing in place ----
    Loader {
        id: inline
        anchors.fill: image
        active: root.inlineVideo || root.inlineGif
        sourceComponent: root.inlineVideo ? videoComp : gifComp
        // Shown once frames arrive; the picture stays underneath until then (and if it fails).
        opacity: item && item.showing ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.smooth } }
    }
    Component {
        id: videoComp
        VideoPlayer {
            readonly property bool showing: playing
            source: root.firstVideo ? root.firstVideo.url : ""
            resolveKind: root.firstVideo ? (root.firstVideo.resolve || "") : ""
            alternates: root.firstVideo ? (root.firstVideo.alternates || []) : []
            fillMode: VideoOutput.PreserveAspectCrop
            forceMuted: true
            quiet: true
            showControls: false
            enabled: false   // taps go to the card
        }
    }
    Component {
        id: gifComp
        AnimatedImage {
            readonly property bool showing: status === Image.Ready
            source: root.previewUrl
            asynchronous: true
            cache: false
            fillMode: Image.PreserveAspectCrop
            playing: true
        }
    }
    // Corner cover over whatever plays in place (the picture has its own).
    Rectangle {
        visible: inline.opacity > 0
        readonly property int bw: Math.ceil(Theme.radiusMedia * 0.45) + 1
        x: image.x - bw
        y: image.y - bw
        width: image.width + 2 * bw
        height: image.height + 2 * bw
        radius: Theme.radiusMedia + bw
        color: "transparent"
        border.width: bw
        border.color: root.backgroundColor
    }

    // ---- Gallery grid: 2 side by side, 3 as one big + two, 4+ as 2x2 with "+N" ----
    Item {
        id: gridBox
        anchors.fill: parent
        visible: root.grid
        readonly property int n: Math.min(4, root.media.length)
        readonly property int gap: 4
        Repeater {
            model: root.grid ? gridBox.n : 0
            delegate: Item {
                id: tile
                required property int index
                readonly property var item: root.media[index]
                readonly property bool isVideo: item.video === true
                readonly property real half: (gridBox.width - gridBox.gap) / 2
                readonly property real halfH: (gridBox.height - gridBox.gap) / 2
                x: index === 0 ? 0 : gridBox.n === 4 ? (index % 2) * (half + gridBox.gap) : half + gridBox.gap
                y: gridBox.n === 4 ? Math.floor(index / 2) * (halfH + gridBox.gap)
                 : gridBox.n === 3 && index === 2 ? halfH + gridBox.gap : 0
                width: gridBox.n === 1 ? gridBox.width : half
                height: gridBox.n === 4 || (gridBox.n === 3 && index > 0) ? halfH : gridBox.height

                RoundedImage {
                    anchors.fill: parent
                    visible: !tile.isVideo
                    source: tile.isVideo ? "" : root.thumbFor(tile.item.url)
                    fallbackSource: tile.isVideo ? "" : (tile.item.fallbackUrl || "")
                    decodeWidth: 640
                    backgroundColor: root.backgroundColor
                }
                VideoThumb {
                    anchors.fill: parent
                    visible: tile.isVideo
                    item: tile.isVideo ? tile.item : ({})
                    // The post's own thumbnail belongs to its first item.
                    fallbackPoster: tile.index === 0 ? root.previewUrl : ""
                    showPlay: !root.covered
                    backgroundColor: root.backgroundColor
                }
                Rectangle {
                    anchors.fill: parent
                    visible: tile.index === 3 && root.media.length > 4 && !root.covered
                    radius: Theme.radiusMedia
                    color: "#99000000"
                    Text {
                        anchors.centerIn: parent
                        text: "+" + (root.media.length - 3)
                        color: "white"
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }
                }
                TapHandler {
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onTapped: root.covered ? root.revealed = true : root.activated(tile.index)
                }
            }
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    TapHandler {
        enabled: !root.grid
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: root.covered ? root.revealed = true : root.activated(0)
    }

    // Spoiler / NSFW cover
    Rectangle {
        anchors.fill: root.grid ? gridBox : image
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
        TapHandler { onTapped: root.revealed = true }
    }

    Chip {
        visible: root.kind === PostListModel.GalleryPost && !root.covered && !root.grid
        anchors.top: image.top
        anchors.right: image.right
        anchors.margins: 10
        iconName: "gallery"
        text: "1 / " + root.galleryCount
        fillColor: "#B3000000"
        textColor: "white"
        border.width: 0
    }

    Icon {
        visible: root.previewUrl === "" && root.kind === PostListModel.GalleryPost && !root.covered && !root.grid
        anchors.centerIn: image
        name: "gallery"
        size: 36
        color: Theme.textMuted
    }

    // Play button, until the video plays in place.
    Rectangle {
        visible: root.kind === PostListModel.VideoPost && !root.covered && !root.grid && inline.opacity < 1
        anchors.centerIn: image
        width: 56
        height: 56
        radius: 28
        color: "#E6FFFFFF"
        Icon { anchors.centerIn: parent; anchors.horizontalCenterOffset: 2; name: "play"; size: 24; color: "#141418" }
    }
    // Other links to the same video (tried automatically if one fails; switchable in the viewer).
    Chip {
        readonly property int count: root.firstVideo && root.firstVideo.alternates ? root.firstVideo.alternates.length + 1 : 1
        visible: !root.grid && root.kind === PostListModel.VideoPost && !root.covered && count > 1
        anchors.top: image.top
        anchors.right: image.right
        anchors.margins: 10
        iconName: "link"
        text: qsTr("%1 sources").arg(count)
        fillColor: "#B3000000"
        textColor: "white"
        border.width: 0
    }

    // While playing in place: a quiet hint that it's muted and opens with sound.
    Rectangle {
        visible: root.inlineVideo && inline.opacity > 0.5
        anchors.right: image.right
        anchors.bottom: image.bottom
        anchors.margins: 10
        width: 30
        height: 30
        radius: 15
        color: "#99000000"
        Icon { anchors.centerIn: parent; name: "volume-off"; size: 16; color: "white" }
    }
}
