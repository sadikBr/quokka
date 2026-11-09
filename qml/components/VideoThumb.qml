import QtQuick

// A still for a video: its poster (e.g. the post's thumbnail); for redgifs, the poster redgifs
// publishes next to the file; otherwise the video's first frame (VideoFrames). A dark tile with a play button when there is no picture.
Item {
    id: root
    property var item: ({})            // media map: url, resolve, poster, ...
    property string fallbackPoster     // used when the item has no poster of its own
    property bool showPlay: true
    property bool showHost: false
    property real decodeWidth: 640
    property color backgroundColor: Theme.bg

    property string lookedUp: ""
    readonly property string poster: (item && item.poster) || fallbackPoster || lookedUp
    readonly property bool hasPicture: poster !== "" && !picture.broken

    // redgifs files are https://media.redgifs.com/Name.mp4 (or Name-mobile.mp4); the poster is
    // Name-poster.jpg beside them.
    function posterFromVideo(mediaUrl) {
        const m = String(mediaUrl).match(/^(https:\/\/[^\/]*redgifs\.com\/[^?]+?)(-mobile|-silent)?\.mp4(\?.*)?$/i)
        return m ? m[1] + "-poster.jpg" : ""
    }
    // No picture at all: look one up (redgifs publishes posters), else grab the first frame.
    property string grabbedFrom: ""   // the video file VideoFrames was asked about
    function grab(fileUrl) {
        grabbedFrom = String(fileUrl)
        const hit = VideoFrames.frameFor(grabbedFrom)
        if (hit !== "")
            lookedUp = hit
        else
            VideoFrames.request(grabbedFrom)
    }
    function useFile(mediaUrl) {
        if (item.resolve === "redgifs") {
            const p = posterFromVideo(mediaUrl)
            if (p !== "") { lookedUp = p; return }
        }
        grab(mediaUrl)
    }
    function lookUp() {
        if (!item || !item.url || item.poster || fallbackPoster)
            return
        if (item.resolve) {
            const hit = MediaResolver.cached(item.url)
            if (hit !== "")
                useFile(hit)
            else
                MediaResolver.resolve(item.url, item.resolve)
            return
        }
        grab(item.url)
    }
    Component.onCompleted: lookUp()
    onItemChanged: { lookedUp = ""; grabbedFrom = ""; lookUp() }
    Connections {
        target: root.item && root.item.resolve && !root.item.poster && !root.fallbackPoster ? MediaResolver : null
        function onResolved(pageUrl, mediaUrl) {
            if (pageUrl === String(root.item.url))
                root.useFile(mediaUrl)
        }
    }
    Connections {
        target: root.grabbedFrom !== "" ? VideoFrames : null
        function onFrameReady(videoUrl, fileUrl) {
            if (videoUrl === root.grabbedFrom)
                root.lookedUp = fileUrl
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMedia
        color: Theme.dark ? "#202027" : "#26262C"
    }
    RoundedImage {
        id: picture
        anchors.fill: parent
        visible: root.poster !== ""
        source: root.poster
        decodeWidth: root.decodeWidth
        showBroken: false
        backgroundColor: root.backgroundColor
        placeholderColor: "transparent"
    }
    Column {
        anchors.centerIn: parent
        spacing: 8
        visible: root.showPlay
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 44
            height: 44
            radius: 22
            color: "#E6FFFFFF"
            Icon { anchors.centerIn: parent; anchors.horizontalCenterOffset: 2; name: "play"; size: 20; color: "#141418" }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.showHost && !root.hasPicture
            text: {
                const m = String(root.item ? root.item.url : "").match(/^https?:\/\/(?:www\.)?([^\/]+)/i)
                return m ? m[1] : ""
            }
            color: "#CCFFFFFF"
            font.pixelSize: Theme.fontSmall
        }
    }
}
