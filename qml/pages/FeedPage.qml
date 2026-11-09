import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A scrolling feed for one subreddit / multireddit / popular.
Page {
    id: page

    // The root feed remembers the last community; pushed feeds (tap on a subreddit) do not.
    property bool isRoot: true
    property string initialFeed: AppSettings.lastFeed
    // What the user picked: "home", "popular", "all", "pics", "pics+aww"...
    property string feedKey: initialFeed.replace(/^\/?r\//i, "")
    readonly property bool isHome: feedKey.toLowerCase() === "home"
    readonly property string feedTitle: {
        const k = feedKey.toLowerCase()
        if (k === "home") return qsTr("Home")
        if (k === "all") return qsTr("All")
        if (k === "local") return qsTr("Local")
        if (k === "popular") return qsTr("Popular")
        if (k.indexOf("+") >= 0) return qsTr("%1 communities").arg(k.split("+").length)
        return Api.community(posts.feed)
    }
    readonly property bool active: StackView.status === StackView.Active || StackView.view === null

    signal menuRequested()
    signal pickerRequested()
    signal backRequested()

    function goTo(feed) {
        feedKey = feed.replace(/^\/?r\//i, "").replace(/\/+$/, "") || "popular"
        if (isRoot)
            AppSettings.lastFeed = feedKey
        list.positionViewAtBeginning()
    }

    background: Rectangle { color: Theme.bg }

    SubredditInfo {
        id: about
        name: page.isHome ? "" : posts.feed
    }
    readonly property bool realSubreddit: about.loaded

    PostListModel {
        id: posts
        // Home: the account's subscriptions when signed in, otherwise the local list.
        feed: page.isHome ? (Account.signedIn ? "subscribed" : Library.homeFeed !== "" ? Library.homeFeed : "all")
                          : page.feedKey
        sort: AppSettings.defaultSort
        showNsfw: AppSettings.showNsfw
        hideBroken: AppSettings.hideBrokenMedia
        nsfwOnly: AppSettings.showNsfw && AppSettings.nsfwOnly
    }

    Shortcut {
        sequences: [StandardKey.Refresh, "Ctrl+R", "R"]
        enabled: page.active
        onActivated: posts.refresh()
    }

    // Keyboard navigation: J/K select, Enter/O open, M media, S save, U go to top.
    property int kbIndex: -1
    Connections {
        target: posts
        function onFeedChanged() { page.kbIndex = -1 }
        function onSortChanged() { page.kbIndex = -1 }
    }
    function step(delta) {
        if (posts.count === 0)
            return
        kbIndex = Math.max(0, Math.min(posts.count - 1, kbIndex + delta))
        list.positionViewAtIndex(kbIndex, ListView.Contain)
    }
    Shortcut { sequences: ["J"]; enabled: page.active; onActivated: page.step(1) }
    Shortcut { sequences: ["K"]; enabled: page.active; onActivated: page.step(-1) }
    Shortcut {
        sequences: ["Return", "Enter", "O"]
        enabled: page.active && page.kbIndex >= 0
        onActivated: Nav.openPost(posts.get(page.kbIndex))
    }
    Shortcut {
        sequences: ["M"]
        enabled: page.active && page.kbIndex >= 0
        onActivated: Nav.openMedia(posts.get(page.kbIndex), 0)
    }
    Shortcut {
        sequences: ["S"]
        enabled: page.active && page.kbIndex >= 0
        onActivated: Library.toggleSaved(posts.get(page.kbIndex))
    }
    Shortcut {
        sequences: ["U"]
        enabled: page.active
        onActivated: { page.kbIndex = -1; list.positionViewAtBeginning() }
    }

    // ---- Header: slim app bar + sort tabs ------------------------------------------------
    readonly property string subtitle: {
        const k = feedKey.toLowerCase()
        if (k === "local") return qsTr("Posts from %1").arg(Api.serviceName)
        if (k === "all" || k === "popular") return qsTr("Everything on %1").arg(Api.serviceName)
        if (isHome)
            return Account.signedIn ? qsTr("Your subscriptions")
                 : Library.subscriptions.length === 0 ? qsTr("Join communities to fill Home")
                 : qsTr("%1 communities").arg(Library.subscriptions.length)
        if (realSubreddit)
            return qsTr("%1 members").arg(Format.compact(about.subscribers))
        return ""
    }

    header: Rectangle {
        implicitHeight: headerCol.implicitHeight
        color: Theme.bg

        ColumnLayout {
            id: headerCol
            width: parent.width
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                RowLayout {
                    // Lines up with the reading column on wide windows.
                    width: Math.min(parent.width - 16, Theme.contentMaxWidth + 16)
                    height: parent.height
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 4

                    RoundButton2 {
                        visible: !page.isRoot || !Nav.sidebarDocked
                        iconName: page.isRoot ? "menu" : "chevron-left"
                        tint: Theme.text
                        tooltip: page.isRoot ? qsTr("Menu") : qsTr("Back")
                        onClicked: page.isRoot ? page.menuRequested() : page.backRequested()
                    }

                    // Feed title: tap to switch community
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 12
                        color: titleHover.hovered ? Theme.hover : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 10
                            Avatar {
                                name: page.isHome ? "Home" : posts.feed
                                imageUrl: page.isHome ? Theme.logo : about.iconUrl
                                size: 32
                                backgroundColor: titleHover.hovered ? Theme.hover : Theme.bg
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                RowLayout {
                                    spacing: 4
                                    Text {
                                        Layout.maximumWidth: Math.max(80, page.width - 240)
                                        text: page.feedTitle
                                        color: Theme.text
                                        font.pixelSize: 18
                                        font.weight: Font.Bold
                                        elide: Text.ElideRight
                                    }
                                    Icon { name: "chevron-down"; size: 16; color: Theme.textMuted }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    visible: text !== ""
                                    text: page.subtitle
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSmall
                                    elide: Text.ElideRight
                                }
                            }
                        }
                        HoverHandler { id: titleHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: page.pickerRequested() }
                    }

                    RoundButton2 {
                        iconName: "reels"
                        tint: Theme.text
                        tooltip: qsTr("Swipe through the pictures and videos")
                        onClicked: Nav.openMediaFeed({ feed: posts.feed, sort: posts.sort, timeRange: posts.timeRange,
                                                       title: page.feedTitle })
                    }
                    RoundButton2 {
                        iconName: "search"
                        tint: Theme.text
                        tooltip: page.realSubreddit ? qsTr("Search %1").arg(Api.community(posts.feed)) : qsTr("Search")
                        onClicked: Nav.openSearch("", page.realSubreddit ? posts.feed : "")
                    }
                    RoundButton2 {
                        id: refreshBtn
                        iconName: "refresh"
                        tint: Theme.text
                        tooltip: qsTr("Refresh")
                        onClicked: posts.refresh()
                        RotationAnimator on rotation {
                            running: posts.loading
                            loops: Animation.Infinite
                            from: 0
                            to: 360
                            duration: 900
                            onRunningChanged: if (!running) refreshBtn.rotation = 0
                        }
                    }
                }
            }

            SortBar {
                Layout.preferredWidth: Math.min(parent.width - 24, Theme.contentMaxWidth)
                Layout.alignment: Qt.AlignHCenter
                sort: posts.sort
                timeRange: posts.timeRange
                onSortPicked: (s) => { posts.sort = s; list.positionViewAtBeginning() }
                onTimeRangePicked: (r) => { posts.timeRange = r; list.positionViewAtBeginning() }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.divider
            }
        }
    }

    // ---- Feed ---------------------------------------------------------------------------
    ListView {
        id: list
        anchors.fill: parent
        model: posts
        spacing: 0
        topMargin: 4
        bottomMargin: 24
        reuseItems: false
        cacheBuffer: 1200
        boundsBehavior: Flickable.DragOverBounds
        flickDeceleration: 2500
        maximumFlickVelocity: 6000

        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)
        readonly property real pull: Math.max(0, -(contentY - originY + topMargin))

        ScrollBar.vertical: ThinScrollBar {}

        onContentYChanged: {
            atTop = contentY - originY < 4
            if (posts.count > 0 && !posts.loading && posts.hasMore
                    && contentY + height > originY + contentHeight - height * 1.5)
                posts.loadMore()
        }
        onDragEnded: if (pull > 90) posts.refresh()

        // The community block above the posts grows once loaded. If we were at the top when it
        // grew, stay at the top (otherwise the list would keep its offset and hide part of it).
        // Only header growth triggers this; scrolling never does.
        property bool atTop: true

        // Community details at the top of a community's feed.
        header: Item {
            width: list.width
            height: page.realSubreddit ? aboutBlock.implicitHeight + 8 : 0
            onHeightChanged: if (list.atTop) Qt.callLater(list.positionViewAtBeginning)
            visible: page.realSubreddit
            ColumnLayout {
                id: aboutBlock
                width: Math.min(parent.width - 24, Theme.contentMaxWidth)
                anchors.horizontalCenter: parent.horizontalCenter
                y: 12
                spacing: 12

                RoundedImage {
                    visible: about.bannerUrl !== ""
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? Math.round(width / 4) : 0
                    source: about.bannerUrl
                    radius: Theme.radiusCard
                    backgroundColor: Theme.bg
                    showBroken: false
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    Avatar {
                        name: posts.feed
                        imageUrl: about.iconUrl
                        size: 56
                        backgroundColor: Theme.bg
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            Layout.fillWidth: true
                            text: about.title !== "" ? about.title : Api.community(posts.feed)
                            color: Theme.text
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: Api.community(about.displayName !== "" ? about.displayName : posts.feed)
                                  + "  ·  " + qsTr("%1 members").arg(Format.compact(about.subscribers))
                                  + (about.activeUsers >= 0 ? "  ·  " + qsTr("%1 active today").arg(Format.compact(about.activeUsers)) : "")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall + 1
                            elide: Text.ElideRight
                        }
                    }
                    JoinButton {
                        name: about.displayName !== "" ? about.displayName : posts.feed
                        communityId: about.communityId
                        fontSize: Theme.fontMeta
                        hpad: 16
                        implicitHeight: 34
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: about.description !== ""
                    text: about.description
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontMeta + 1
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    lineHeight: 1.2
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.divider
                }
            }
        }

        delegate: PostCard {
            width: list.width
            view: list
            autoplay: page.active && AppSettings.autoplayFeed
            highlighted: index === page.kbIndex
            saved: Library.savedRevision >= 0 && Library.isSaved(postId)
            visited: Library.historyRevision >= 0 && Library.isVisited(postId)
            onToggleSave: Library.toggleSaved(posts.get(index))
            onOpenPost: { page.kbIndex = index; Nav.openPost(posts.get(index)) }
            onOpenMedia: (i) => Nav.openMedia(posts.get(index), i)
            onOpenSubreddit: (name) => {
                if (name.toLowerCase() !== posts.feed.toLowerCase())
                    Nav.openSubreddit(name)
            }
            onOpenLink: (link) => Nav.openLink(link)
        }

        footer: Item {
            width: list.width
            height: footerLoader.item ? footerLoader.item.implicitHeight + 24 : 0
            Loader {
                id: footerLoader
                width: list.contentWidth2
                anchors.horizontalCenter: parent.horizontalCenter
                y: 12
                sourceComponent: {
                    if (posts.count === 0 && posts.loading) return skeletons
                    if (posts.count === 0 && posts.error !== "") return errorState
                    if (posts.count === 0) return emptyState
                    if (posts.error !== "") return inlineError
                    if (posts.loading) return dots
                    if (!posts.hasMore) return theEnd
                    return null
                }
            }
        }
    }
    WheelScroller {
        flickable: list
        enabled: list.visible
    }

    // Pull-to-refresh bubble
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.min(list.pull, 110) - height + 4
        visible: list.pull > 4
        width: 44
        height: 44
        radius: 22
        color: list.pull > 90 ? Theme.accent : Theme.surface
        border.width: 1
        border.color: Theme.border
        Icon {
            anchors.centerIn: parent
            name: "refresh"
            size: 22
            rotation: list.pull * 3
            color: list.pull > 90 ? Theme.textOnAccent : Theme.accent
        }
    }

    Component {
        id: skeletons
        Column {
            spacing: 0
            Repeater {
                model: 4
                SkeletonCard {
                    required property int index
                    width: parent ? parent.width : 0
                    withMedia: index % 2 === 0
                }
            }
        }
    }
    Component {
        id: errorState
        EmptyState {
            title: qsTr("Oops, a tumble!")
            message: posts.error
            actionText: qsTr("Try again")
            onAction: posts.refresh()
        }
    }
    Component {
        id: emptyState
        EmptyState {
            title: qsTr("So quiet here…")
            message: qsTr("No posts to show. Try another sort, or another community.")
            actionText: qsTr("Refresh")
            onAction: posts.refresh()
        }
    }
    Component {
        id: inlineError
        Rectangle {
            implicitHeight: errRow.implicitHeight + 24
            radius: Theme.radiusMedia
            color: Theme.alpha(Theme.nsfw, 0.12)
            RowLayout {
                id: errRow
                anchors.fill: parent
                anchors.margins: 12
                Text {
                    Layout.fillWidth: true
                    text: posts.error
                    color: Theme.text
                    font.pixelSize: Theme.fontMeta
                    wrapMode: Text.Wrap
                }
                Chip {
                    text: qsTr("Retry")
                    clickable: true
                    selected: true
                    onClicked: posts.refresh()
                }
            }
        }
    }
    Component {
        id: dots
        Item {
            implicitHeight: 40
            LoadingDots { anchors.centerIn: parent }
        }
    }
    Component {
        id: theEnd
        Column {
            spacing: 6
            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: Theme.logo
                sourceSize: Qt.size(48, 48)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("You reached the bottom. Go touch some grass!")
                color: Theme.textMuted
                font.pixelSize: Theme.fontMeta
            }
        }
    }
}
