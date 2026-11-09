import QtQuick
import QtQuick.Controls

// Swipe-through media: one post per screen, swipe up for the next one, endlessly (only posts
// with pictures or videos). Videos play when they're on screen; galleries swipe sideways.
// Opened with the same feed / search / user as the page it was opened from.
Page {
    id: page
    // feed, sort, timeRange, searchQuery, userName, title
    property var params: ({})
    readonly property bool active: StackView.status === StackView.Active
    readonly property bool immersive: true   // dark system bars (see Main)
    property bool chrome: true               // overlays shown (tap a picture to hide them)
    // Only videos and GIFs (no photos); remembered between visits.
    property bool videosOnly: AppSettings.value("media/feedVideosOnly", false) === true
                              || AppSettings.value("media/feedVideosOnly", false) === "true"
    onVideosOnlyChanged: {
        AppSettings.setValue("media/feedVideosOnly", videosOnly)
        pager.currentIndex = -1   // the list reloads: start again at the top
    }

    function back() { page.StackView.view.pop() }

    background: Rectangle { color: "#000000" }

    PostListModel {
        id: posts
        mediaOnly: true
        source: page.params.source || ""
        animatedOnly: page.videosOnly
        feed: page.params.feed || "all"
        sort: page.params.sort || AppSettings.defaultSort
        timeRange: page.params.timeRange || "day"
        searchQuery: page.params.searchQuery || ""
        userName: page.params.userName || ""
        showNsfw: AppSettings.showNsfw
        hideBroken: AppSettings.hideBrokenMedia
        nsfwOnly: AppSettings.showNsfw && AppSettings.nsfwOnly
    }

    function next() { if (pager.currentIndex < posts.count - 1) pager.incrementCurrentIndex() }
    function previous() { if (pager.currentIndex > 0) pager.decrementCurrentIndex() }

    Shortcut { sequences: ["Down", "J", "PgDown"]; enabled: page.active; onActivated: page.next() }
    Shortcut { sequences: ["Up", "K", "PgUp"]; enabled: page.active; onActivated: page.previous() }
    Shortcut { sequences: ["Right", "L"]; enabled: page.active; onActivated: pager.currentItem && pager.currentItem.step(1) }
    Shortcut { sequences: ["Left", "H"]; enabled: page.active; onActivated: pager.currentItem && pager.currentItem.step(-1) }
    Shortcut { sequences: ["Space"]; enabled: page.active; onActivated: pager.currentItem && pager.currentItem.togglePlay() }
    Shortcut { sequences: ["V"]; enabled: page.active; onActivated: page.videosOnly = !page.videosOnly }
    Shortcut { sequences: ["M"]; enabled: page.active; onActivated: AppSettings.videoMuted = !AppSettings.videoMuted }
    Shortcut { sequences: ["C"]; enabled: page.active && posts.count > 0 && !(page.params.source); onActivated: Nav.openPost(posts.get(pager.currentIndex)) }

    ListView {
        id: pager
        anchors.fill: parent
        model: posts
        orientation: ListView.Vertical
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 260
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: height          // the next post is ready before you get there
        interactive: !(currentItem && currentItem.zoomed)
        onCurrentIndexChanged: if (currentIndex >= posts.count - 4) posts.loadMore()
        onCountChanged: {
            // Posts arrive after the page opens: start at the first one.
            if (count > 0 && (currentIndex < 0 || currentIndex >= count))
                currentIndex = 0
            if (count > 0 && currentIndex >= count - 4)
                posts.loadMore()
        }

        delegate: Item {
            id: slot
            required property int index
            required property var model
            width: pager.width
            height: pager.height
            readonly property bool isCurrent: ListView.isCurrentItem
            // Load the current post and its neighbours only.
            readonly property bool near: Math.abs(index - pager.currentIndex) <= 1
            readonly property var items: {
                let m = model.media || []
                // Videos mode: a gallery shows only its videos and GIFs.
                if (page.videosOnly)
                    m = m.filter(x => x.video || x.animated || /\.(gif|gifv|mp4|webm|m3u8)(\?|$)/i.test(x.url))
                if (m.length > 0) return m
                return model.previewUrl ? [{ url: model.previewUrl, fallbackUrl: model.previewFallbackUrl }] : []
            }
            readonly property bool zoomed: gallery.currentItem ? gallery.currentItem.zoomed === true : false
            readonly property int itemIndex: gallery.currentIndex
            readonly property bool currentIsVideo: gallery.currentItem ? gallery.currentItem.isVideo === true : false

            function step(delta) {
                delta > 0 ? gallery.incrementCurrentIndex() : gallery.decrementCurrentIndex()
            }
            function togglePlay() { if (gallery.currentItem) gallery.currentItem.togglePlay() }

            // Gallery: sideways pager (a single item is a pager of one).
            ListView {
                id: gallery
                anchors.fill: parent
                orientation: ListView.Horizontal
                snapMode: ListView.SnapOneItem
                highlightRangeMode: ListView.StrictlyEnforceRange
                highlightMoveDuration: 220
                boundsBehavior: Flickable.StopAtBounds
                interactive: slot.items.length > 1 && !slot.zoomed
                model: slot.near ? slot.items : []
                cacheBuffer: width

                delegate: Item {
                    id: cell
                    required property var modelData
                    required property int index
                    width: gallery.width
                    height: gallery.height
                    readonly property bool isVideo: modelData.video === true
                                                    || /\.(m3u8|mp4|webm|mov|m4v)(\?|$)/i.test(modelData.url)
                    readonly property bool shown: slot.isCurrent && ListView.isCurrentItem && page.active
                    readonly property bool zoomed: loader.item && loader.item.zoomed === true
                    function togglePlay() { if (isVideo && loader.item) loader.item.togglePlay() }

                    Loader {
                        id: loader
                        anchors.fill: parent
                        active: Math.abs(cell.index - gallery.currentIndex) <= 1
                        sourceComponent: cell.isVideo ? videoComp : imageComp
                    }
                    Component {
                        id: imageComp
                        ZoomableImage {
                            source: cell.modelData.url
                            fallbackSource: cell.modelData.fallbackUrl || ""
                            placeholderSource: slot.model.media && slot.model.media.length > 0
                                               && slot.model.media[0].url === cell.modelData.url ? (slot.model.previewUrl || "") : ""
                            animated: /\.gif(\?|$)/i.test(cell.modelData.url)
                            onTapped: page.chrome = !page.chrome
                        }
                    }
                    Component {
                        id: videoComp
                        VideoPlayer {
                            source: cell.modelData.url
                            resolveKind: cell.modelData.resolve || ""
                            alternates: cell.modelData.alternates || []
                            // The post's thumbnail belongs to its own first item; else a grabbed first frame.
                            poster: cell.modelData.poster
                                    || (slot.model.media && slot.model.media.length > 0
                                        && slot.model.media[0].url === cell.modelData.url ? (slot.model.previewUrl || "") : "")
                                    || VideoFrames.frameFor(String(cell.modelData.url))
                            active: cell.shown
                            showControls: false
                            onTapped: togglePlay()
                            // Every link failed: have it checked (dead links are dropped from the
                            // list, the next post slides in) and move on either way.
                            onFailedChanged: {
                                if (!failed)
                                    return
                                const urls = [String(cell.modelData.url)].concat(
                                    (cell.modelData.alternates || []).map(a => String(a.url)))
                                posts.reportBroken(slot.model.postId, urls)
                                if (cell.shown)
                                    skipTimer.restart()
                            }
                            Timer {
                                id: skipTimer
                                interval: 2000
                                onTriggered: if (cell.shown && pager.currentIndex === slot.index) page.next()
                            }
                        }
                    }
                    // Paused: a big play mark (tap again to continue).
                    Rectangle {
                        visible: cell.isVideo && loader.item && loader.item.paused === true && cell.shown
                        anchors.centerIn: parent
                        width: 72
                        height: 72
                        radius: 36
                        color: "#99000000"
                        Icon { anchors.centerIn: parent; anchors.horizontalCenterOffset: 3; name: "play"; size: 32; color: "white" }
                    }
                }
            }

            // NSFW / spoiler cover, like in the feed.
            property bool revealed: false
            Rectangle {
                anchors.fill: parent
                visible: ((slot.model.nsfw && AppSettings.coverNsfw) || slot.model.spoiler) && !slot.revealed
                color: "#141418"
                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "eye-off"; size: 34; color: "#B3FFFFFF" }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: slot.model.nsfw ? qsTr("NSFW · tap to show") : qsTr("Spoiler · tap to reveal")
                        color: "#B3FFFFFF"
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.Bold
                    }
                }
                TapHandler { onTapped: slot.revealed = true }
            }

            // ---- Post info over the bottom ----
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: info.implicitHeight + 64
                visible: page.chrome
                gradient: Gradient {
                    GradientStop { position: 0; color: "#00000000" }
                    GradientStop { position: 1; color: "#CC000000" }
                }
            }
            Column {
                id: info
                visible: page.chrome
                anchors.left: parent.left
                anchors.right: actions.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                anchors.bottomMargin: 20
                spacing: 6
                Row {
                    spacing: 8
                    Avatar {
                        name: slot.model.source ? slot.model.source : slot.model.subreddit
                        size: 24
                        backgroundColor: "#000000"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        readonly property string site: slot.model.source === "redgifs" ? "Redgifs"
                                                     : slot.model.source === "eporner" ? "Eporner" : ""
                        text: (site !== "" ? site + (slot.model.author ? "  ·  @" + slot.model.author : "")
                                           : Api.community(slot.model.subreddit))
                              + "  ·  " + Format.relativeTime(slot.model.createdUtc)
                        color: "white"
                        font.pixelSize: Theme.fontSmall + 1
                        font.weight: Font.DemiBold
                        TapHandler {
                            onTapped: {
                                if (slot.model.source === "redgifs" && slot.model.author)
                                    Nav.openSource({ source: "redgifs", user: slot.model.author })
                                else if (slot.model.source)
                                    Nav.openSource({ source: slot.model.source })
                                else
                                    Nav.openSubreddit(slot.model.subreddit)
                            }
                        }
                    }
                }
                Text {
                    id: titleText
                    width: parent.width
                    property bool expanded: false
                    text: slot.model.title
                    textFormat: Text.PlainText
                    color: "white"
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                    maximumLineCount: expanded ? 12 : 2
                    elide: Text.ElideRight
                    lineHeight: 1.15
                    TapHandler { onTapped: titleText.expanded = !titleText.expanded }
                }
                // Gallery position
                Row {
                    visible: slot.items.length > 1
                    spacing: 5
                    Repeater {
                        model: Math.min(slot.items.length, 12)
                        Rectangle {
                            required property int index
                            width: index === slot.itemIndex ? 16 : 6
                            height: 6
                            radius: 3
                            color: index === slot.itemIndex ? "white" : "#80FFFFFF"
                            Behavior on width { NumberAnimation { duration: Theme.quick } }
                        }
                    }
                }
            }
            Column {
                id: actions
                visible: page.chrome
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 10
                anchors.bottomMargin: 20
                spacing: 14
                SideButton {
                    iconName: "upvote"
                    label: Format.compact(slot.model.score)
                }
                SideButton {
                    visible: !slot.model.source
                    iconName: "comment"
                    label: Format.compact(slot.model.numComments)
                    onClicked: Nav.openPost(posts.get(slot.index))
                }
                SideButton {
                    visible: slot.currentIsVideo
                    iconName: AppSettings.videoMuted ? "volume-off" : "volume"
                    label: ""
                    onClicked: AppSettings.videoMuted = !AppSettings.videoMuted
                }
                SideButton {
                    iconName: "external"
                    label: ""
                    onClicked: slot.model.source ? Qt.openUrlExternally(slot.model.permalink)
                                                 : Nav.openLink(slot.model.url !== "" ? slot.model.url : Api.webUrl(slot.model.permalink))
                }
            }
        }
    }

    // Mouse wheel / touchpad: one post per scroll step.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        property real pending: 0
        property double lastStep: 0
        onWheel: (wheel) => {
            if (pager.currentItem && pager.currentItem.zoomed) {
                wheel.accepted = false
                return
            }
            pending += wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y * 2
            const now = Date.now()
            if (Math.abs(pending) >= 120 && now - lastStep > 300) {
                pending < 0 ? page.next() : page.previous()
                lastStep = now
                pending = 0
            } else if (now - lastStep <= 300) {
                pending = 0   // a touchpad's tail of small events after a step
            }
        }
    }

    // ---- Top bar ----
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        visible: page.chrome
        gradient: Gradient {
            GradientStop { position: 0; color: "#AA000000" }
            GradientStop { position: 1; color: "#00000000" }
        }
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            RoundButton2 {
                iconName: "chevron-left"
                tint: "white"
                fill: "#66000000"
                tooltip: qsTr("Back")
                onClicked: page.back()
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: page.params.title || ""
                color: "white"
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                width: Math.min(implicitWidth, page.width - 230)
            }
        }
    }

    // Photos and videos, or videos (and GIFs) only.
    Rectangle {
        visible: page.chrome
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 14
        height: 36
        width: modeRow.implicitWidth + 8
        radius: 18
        color: "#66000000"
        Row {
            id: modeRow
            anchors.centerIn: parent
            spacing: 2
            Repeater {
                model: [qsTr("All"), qsTr("Videos")]
                Rectangle {
                    required property int index
                    required property string modelData
                    readonly property bool selected: (index === 1) === page.videosOnly
                    width: modeText.implicitWidth + 24
                    height: 28
                    radius: 14
                    color: selected ? "white" : "transparent"
                    Text {
                        id: modeText
                        anchors.centerIn: parent
                        text: parent.modelData
                        color: parent.selected ? "#141418" : "white"
                        font.pixelSize: Theme.fontSmall + 1
                        font.weight: Font.DemiBold
                    }
                    TapHandler { onTapped: page.videosOnly = (parent.index === 1) }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
            }
        }
    }

    LoadingDots {
        anchors.centerIn: parent
        visible: posts.count === 0 && posts.loading
    }
    Text {
        anchors.centerIn: parent
        width: parent.width - 48
        visible: posts.count === 0 && !posts.loading
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: posts.error !== "" ? posts.error
            : page.videosOnly ? qsTr("No videos or GIFs here.") : qsTr("No pictures or videos here.")
        color: "#CCFFFFFF"
        font.pixelSize: Theme.fontBody
    }

    // Round icon with a count under it, TikTok-style.
    component SideButton: Column {
        id: side
        property string iconName
        property string label
        signal clicked()
        spacing: 2
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 46
            height: 46
            radius: 23
            color: sideHover.hovered ? "#80000000" : "#59000000"
            Icon { anchors.centerIn: parent; name: side.iconName; size: 22; color: "white" }
            HoverHandler { id: sideHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: side.clicked() }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: side.label !== ""
            text: side.label
            color: "white"
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
        }
    }
}
