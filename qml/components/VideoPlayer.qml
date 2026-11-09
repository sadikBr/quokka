import QtQuick
import QtQuick.Controls
import QtMultimedia

// Plays a video (HLS stream or MP4/WebM file), with simple controls.
Item {
    id: root
    property url source
    property url fallbackSource
    property url poster              // still picture shown when the video can't play
    // Set for video pages (e.g. "redgifs"): `source` is the page; the file is looked up first.
    property string resolveKind
    property url resolvedSource
    property string resolveError
    readonly property bool resolving: resolveKind !== "" && resolvedSource.toString() === "" && resolveError === ""
    readonly property url playSource: resolveKind !== "" ? resolvedSource : source
    property bool active: true
    property bool loop: true
    property bool showControls: true
    signal tapped()

    property bool triedFallback: false
    readonly property bool failed: resolveError !== ""
                                   || (player.error !== MediaPlayer.NoError
                                       && triedFallback === (fallbackSource.toString() !== ""))

    function retry() {
        resolveError = ""
        triedFallback = false
        player.stop()
        if (resolveKind !== "" && resolvedSource.toString() === "") {
            MediaResolver.resolve(source, resolveKind)
            return
        }
        player.source = ""
        player.source = Qt.binding(() => root.playSource)
        player.play()
    }

    onActiveChanged: active ? player.play() : player.pause()
    Component.onCompleted: {
        if (resolveKind !== "") {
            const hit = MediaResolver.cached(source)
            if (hit !== "")
                resolvedSource = hit
            else
                MediaResolver.resolve(source, resolveKind)
        }
        if (active)
            player.play()
    }
    Connections {
        target: root.resolveKind !== "" ? MediaResolver : null
        function onResolved(pageUrl, mediaUrl) {
            if (pageUrl === root.source.toString()) {
                root.resolvedSource = mediaUrl
                if (root.active)
                    Qt.callLater(player.play)
            }
        }
        function onFailed(pageUrl, message) {
            if (pageUrl === root.source.toString())
                root.resolveError = message
        }
    }

    MediaPlayer {
        id: player
        source: root.playSource
        loops: root.loop ? MediaPlayer.Infinite : 1
        videoOutput: video
        audioOutput: AudioOutput { id: audio; muted: AppSettings.videoMuted }
        onErrorOccurred: {
            if (!root.triedFallback && root.fallbackSource.toString() !== "") {
                root.triedFallback = true
                player.source = root.fallbackSource
                player.play()
            }
        }
    }

    RoundedImage {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        visible: root.failed && root.poster.toString() !== ""
        source: visible ? root.poster : ""
        fillMode: Image.PreserveAspectFit
        radius: 0
        showBroken: false
        backgroundColor: "transparent"
        placeholderColor: "transparent"
        opacity: 0.35
    }

    VideoOutput {
        id: video
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    LoadingDots {
        anchors.centerIn: parent
        visible: root.resolving || player.mediaStatus === MediaPlayer.LoadingMedia
                 || player.mediaStatus === MediaPlayer.BufferingMedia
                 || player.mediaStatus === MediaPlayer.StalledMedia
    }

    // Why it can't play, with a retry and a way out to the original page.
    Column {
        anchors.centerIn: parent
        width: parent.width - 48
        visible: root.failed
        spacing: 14
        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "video-off"; size: 36; color: "white" }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: root.resolveError !== "" ? root.resolveError
                : MediaResolver.describePlaybackError(player.errorString, player.source.toString())
            color: "white"
            font.pixelSize: Theme.fontBody
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10
            Chip {
                text: qsTr("Try again")
                iconName: "refresh"
                clickable: true
                onClicked: root.retry()
            }
            Chip {
                text: qsTr("Open the page")
                iconName: "external"
                clickable: true
                selected: true
                onClicked: Qt.openUrlExternally(root.source)
            }
        }
    }

    TapHandler {
        onSingleTapped: root.tapped()
        onDoubleTapped: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    }

    // Controls
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        height: 48
        radius: 24
        color: "#B3000000"
        visible: root.showControls
        opacity: visible ? 1 : 0

        Row {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 12
            spacing: 8

            RoundButton2 {
                anchors.verticalCenter: parent.verticalCenter
                size: 36
                fill: Theme.accent
                tint: "white"
                iconName: player.playbackState === MediaPlayer.PlayingState ? "pause" : "play"
                tooltip: qsTr("Play / pause")
                onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
            }
            Slider {
                id: seek
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 36 - 36 - timeText.width - 3 * parent.spacing - 18
                from: 0
                to: Math.max(1, player.duration)
                value: pressed ? value : player.position
                enabled: player.seekable
                onMoved: player.position = value
            }
            Text {
                id: timeText
                anchors.verticalCenter: parent.verticalCenter
                text: root.formatTime(player.position) + " / " + root.formatTime(player.duration)
                color: "white"
                font.pixelSize: Theme.fontSmall
            }
            RoundButton2 {
                anchors.verticalCenter: parent.verticalCenter
                size: 36
                fill: "transparent"
                tint: "white"
                iconName: AppSettings.videoMuted ? "volume-off" : "volume"
                tooltip: qsTr("Mute")
                onClicked: AppSettings.videoMuted = !AppSettings.videoMuted
            }
        }
    }

    function formatTime(ms) {
        const s = Math.floor(ms / 1000)
        const m = Math.floor(s / 60)
        return m + ":" + String(s % 60).padStart(2, "0")
    }
}
