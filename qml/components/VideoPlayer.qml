import QtQuick
import QtQuick.Controls
import QtMultimedia

// Plays a video (HLS stream or MP4/WebM file), with simple controls.
Item {
    id: root
    property url source
    property url fallbackSource
    property var alternates: []      // other links to the same video: [{url, resolve}], tried in turn
    property url poster              // still picture shown when the video can't play
    // Set for video pages (e.g. "redgifs"): `source` is the page; the file is looked up first.
    property string resolveKind
    property bool active: true
    property bool loop: true
    property bool showControls: true
    property bool forceMuted: false            // e.g. playing in place in the feed
    property bool quiet: false                 // no loading or error overlays (the caller has a poster)
    property alias fillMode: video.fillMode
    readonly property bool playing: player.playbackState === MediaPlayer.PlayingState && player.hasVideo
                                    && (player.mediaStatus === MediaPlayer.BufferedMedia
                                        || player.mediaStatus === MediaPlayer.LoadedMedia)
    signal tapped()

    // Every link to this video, in the order they're tried.
    readonly property var sources: {
        const out = [{ url: String(source), resolve: resolveKind }]
        const add = (u, r) => { if (u !== "" && !out.some(o => o.url === u)) out.push({ url: u, resolve: r || "" }) }
        add(String(fallbackSource), "")
        for (const alt of (alternates || []))
            if (alt && alt.url) add(String(alt.url), alt.resolve)
        return out
    }
    property int sourceIndex: 0
    property var tried: ({})                   // indices already tried (reassigned, never mutated)
    readonly property var current: sources[Math.min(sourceIndex, sources.length - 1)]
    readonly property string currentHost: {
        const m = String(current.url).match(/^https?:\/\/(?:www\.)?([^\/]+)/i)
        return m ? m[1] : ""
    }
    property url resolvedSource
    property string resolveError
    property bool playerFailed: false
    property string playerError
    readonly property bool resolving: current.resolve !== "" && resolvedSource.toString() === "" && resolveError === ""
    readonly property url playSource: current.resolve !== "" ? resolvedSource : current.url
    readonly property bool currentFailed: resolveError !== "" || playerFailed
    // Shown as failed only once every link has been tried.
    readonly property bool failed: currentFailed && nextUntried() < 0

    function nextUntried() {
        const t = tried
        for (let i = 1; i <= sources.length; ++i) {
            const j = (sourceIndex + i) % sources.length
            if (!t[j]) return j
        }
        return -1
    }
    function useSource(i) {
        const t = Object.assign({}, tried)
        t[i] = true
        tried = t
        player.stop()
        sourceIndex = i
        resolveError = ""
        playerFailed = false
        playerError = ""
        resolvedSource = ""
        if (current.resolve !== "") {
            const hit = MediaResolver.cached(current.url)
            if (hit === "") {
                MediaResolver.resolve(current.url, current.resolve)
                return
            }
            resolvedSource = hit
        }
        if (active)
            Qt.callLater(player.play)
    }
    // Pick a link by hand (or try the current one again): every link is fair game again after it.
    function switchTo(i) {
        tried = ({})
        useSource(i)
    }
    function retry() { switchTo(sourceIndex) }
    function togglePlay() {
        player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    }
    readonly property bool paused: player.playbackState === MediaPlayer.PausedState

    onCurrentFailedChanged: {
        if (!currentFailed)
            return
        const next = nextUntried()
        if (next >= 0)
            Qt.callLater(() => root.useSource(next))
    }
    onActiveChanged: active ? player.play() : player.pause()
    Component.onCompleted: useSource(0)
    Connections {
        target: root.current.resolve !== "" ? MediaResolver : null
        function onResolved(pageUrl, mediaUrl) {
            if (pageUrl === String(root.current.url)) {
                root.resolvedSource = mediaUrl
                if (root.active)
                    Qt.callLater(player.play)
            }
        }
        function onFailed(pageUrl, message) {
            if (pageUrl === String(root.current.url))
                root.resolveError = message
        }
    }

    MediaPlayer {
        id: player
        source: root.playSource
        loops: root.loop ? MediaPlayer.Infinite : 1
        videoOutput: video
        audioOutput: AudioOutput { id: audio; muted: root.forceMuted || AppSettings.videoMuted }
        onErrorOccurred: (error, message) => {
            root.playerError = message
            root.playerFailed = true
        }
    }

    RoundedImage {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        // Until the first frame arrives (and dimmed if the video can't play).
        visible: root.poster.toString() !== "" && !root.quiet && (root.failed || !root.playing)
        source: visible ? root.poster : ""
        fillMode: Image.PreserveAspectFit
        radius: 0
        showBroken: false
        backgroundColor: "transparent"
        placeholderColor: "transparent"
        opacity: root.failed ? 0.35 : 1
    }

    VideoOutput {
        id: video
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    LoadingDots {
        anchors.centerIn: parent
        visible: !root.quiet && (root.resolving || player.mediaStatus === MediaPlayer.LoadingMedia
                 || player.mediaStatus === MediaPlayer.BufferingMedia
                 || player.mediaStatus === MediaPlayer.StalledMedia)
    }

    // Why it can't play, with a retry and a way out to the original page.
    Column {
        anchors.centerIn: parent
        width: parent.width - 48
        visible: root.failed && !root.quiet
        spacing: 14
        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "video-off"; size: 36; color: "white" }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: root.resolveError !== "" ? root.resolveError
                : MediaResolver.describePlaybackError(root.playerError, player.source.toString())
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
                onClicked: Qt.openUrlExternally(root.current.url)
            }
        }
    }

    TapHandler {
        onSingleTapped: root.tapped()
        onDoubleTapped: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    }

    // While the controls are hidden: a thin progress line along the bottom.
    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: !root.showControls && !root.quiet && player.duration > 0
        height: 3
        width: parent.width * Math.min(1, player.position / Math.max(1, player.duration))
        color: Theme.accent
        opacity: 0.9
    }

    // Several links to this video: say which one plays, and switch to the next on click.
    Chip {
        anchors.right: parent.right
        anchors.bottom: controls.visible ? controls.top : parent.bottom
        anchors.margins: 12
        visible: root.sources.length > 1 && !root.quiet && (root.showControls || root.failed)
        text: qsTr("Source %1 / %2 · %3").arg(root.sourceIndex + 1).arg(root.sources.length).arg(root.currentHost)
        iconName: "refresh"
        fillColor: "#B3000000"
        textColor: "white"
        border.width: 0
        clickable: true
        onClicked: root.switchTo((root.sourceIndex + 1) % root.sources.length)
    }

    // Controls
    Rectangle {
        id: controls
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
