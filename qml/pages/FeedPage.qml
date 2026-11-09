import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A scrolling feed for one subreddit / multireddit / popular.
Page {
    id: page

    // The root feed remembers the last community; pushed feeds (tap on a subreddit) do not.
    property bool isRoot: true
    property string initialFeed: AppSettings.lastFeed
    readonly property bool active: StackView.status === StackView.Active || StackView.view === null

    signal menuRequested()
    signal pickerRequested()
    signal backRequested()

    function goTo(feed) {
        posts.feed = feed
        if (isRoot)
            AppSettings.lastFeed = posts.feed
        list.positionViewAtBeginning()
    }

    background: Rectangle { color: Theme.bg }

    SubredditInfo {
        id: about
        name: posts.feed
    }
    readonly property bool realSubreddit: about.loaded

    PostListModel {
        id: posts
        feed: page.initialFeed
        sort: AppSettings.defaultSort
        showNsfw: AppSettings.showNsfw
    }

    Shortcut {
        sequences: [StandardKey.Refresh, "Ctrl+R"]
        enabled: page.active
        onActivated: posts.refresh()
    }

    // ---- Header -------------------------------------------------------------------------
    header: Rectangle {
        id: headerBar
        implicitHeight: headerCol.implicitHeight + 28
        color: "transparent"

        // Gradient blob with rounded bottom corners
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: -40          // hide the top rounded corners
            radius: 28
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: about.color !== "" ? about.color : Theme.berry }
                GradientStop { position: 1.0; color: Theme.caramel }
            }
            // Playful confetti dots
            Repeater {
                model: [
                    { x: 0.70, y: 0.52, s: 46, o: 0.16 }, { x: 0.88, y: 0.68, s: 22, o: 0.24 },
                    { x: 0.56, y: 0.80, s: 14, o: 0.30 }, { x: 0.62, y: 0.18, s: 10, o: 0.35 }
                ]
                delegate: Rectangle {
                    required property var modelData
                    x: parent.width * modelData.x
                    y: parent.height * modelData.y
                    width: modelData.s
                    height: modelData.s
                    radius: width / 2
                    color: "white"
                    opacity: modelData.o
                }
            }
        }

        ColumnLayout {
            id: headerCol
            width: Math.min(parent.width - 32, Theme.contentMaxWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                RoundButton2 {
                    iconName: page.isRoot ? "menu" : "chevron-down"
                    rotation: page.isRoot ? 0 : 90
                    fill: Theme.alpha("white", 0.22)
                    tint: "white"
                    tooltip: page.isRoot ? qsTr("Menu") : qsTr("Back")
                    onClicked: page.isRoot ? page.menuRequested() : page.backRequested()
                }
                Item { Layout.fillWidth: true }
                RoundButton2 {
                    iconName: "search"
                    fill: Theme.alpha("white", 0.22)
                    tint: "white"
                    tooltip: page.realSubreddit ? qsTr("Search r/%1").arg(posts.feed) : qsTr("Search")
                    onClicked: Nav.openSearch("", page.realSubreddit ? posts.feed : "")
                }
                RoundButton2 {
                    id: refreshBtn
                    iconName: "refresh"
                    fill: Theme.alpha("white", 0.22)
                    tint: "white"
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

            // Big feed title: tap to switch community
            Item {
                Layout.fillWidth: true
                implicitHeight: titleRow.implicitHeight
                Row {
                    id: titleRow
                    width: parent.width
                    spacing: 12
                    Rectangle {
                        id: titleAvatar
                        width: 52
                        height: 52
                        radius: 26
                        color: "white"
                        Avatar {
                            anchors.centerIn: parent
                            name: posts.feed
                            imageUrl: about.iconUrl
                            backgroundColor: "white"
                            size: 44
                        }
                    }
                    Column {
                        width: titleRow.width - titleAvatar.width - titleRow.spacing
                        anchors.verticalCenter: parent.verticalCenter
                        Row {
                            spacing: 4
                            Text {
                                width: Math.min(implicitWidth, titleRow.width - titleAvatar.width - 50)
                                text: "r/" + posts.feed
                                color: "white"
                                font.pixelSize: 28
                                font.weight: Font.Black
                                elide: Text.ElideRight
                            }
                            Icon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: "chevron-down"
                                size: 22
                                color: "white"
                            }
                        }
                        Text {
                            text: page.realSubreddit
                                ? qsTr("%1 members").arg(Format.compact(about.subscribers))
                                  + (about.activeUsers >= 0 ? qsTr("  ·  %1 online").arg(Format.compact(about.activeUsers)) : "")
                                : posts.count === 1 ? qsTr("1 post loaded")
                                : posts.count > 1 ? qsTr("%1 posts loaded").arg(posts.count)
                                : (posts.loading ? qsTr("Fetching the good stuff…") : " ")
                            color: Theme.alpha("white", 0.85)
                            font.pixelSize: Theme.fontMeta
                            font.weight: Font.DemiBold
                        }
                    }
                }
                TapHandler { onTapped: page.pickerRequested() }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
            }

            Text {
                Layout.fillWidth: true
                visible: page.realSubreddit && about.description !== ""
                text: about.description
                color: Theme.alpha("white", 0.92)
                font.pixelSize: Theme.fontMeta
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }

    // ---- Feed ---------------------------------------------------------------------------
    ListView {
        id: list
        anchors.fill: parent
        model: posts
        spacing: 14
        topMargin: 14
        bottomMargin: 24
        reuseItems: false
        cacheBuffer: 1200
        boundsBehavior: Flickable.DragOverBounds
        flickDeceleration: 2500
        maximumFlickVelocity: 6000

        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)
        readonly property real pull: Math.max(0, -(contentY - originY + topMargin))

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        onContentYChanged: {
            if (posts.count > 0 && !posts.loading && posts.hasMore
                    && contentY + height > originY + contentHeight - height * 1.5)
                posts.loadMore()
        }
        onDragEnded: if (pull > 90) posts.refresh()

        header: Item {
            width: list.width
            height: sortBar.implicitHeight + 6
            SortBar {
                id: sortBar
                width: list.contentWidth2
                anchors.horizontalCenter: parent.horizontalCenter
                sort: posts.sort
                timeRange: posts.timeRange
                onSortPicked: (s) => { posts.sort = s; list.positionViewAtBeginning() }
                onTimeRangePicked: (r) => { posts.timeRange = r; list.positionViewAtBeginning() }
            }
        }

        delegate: PostCard {
            width: list.contentWidth2
            x: (list.width - width) / 2
            onOpenPost: Nav.openPost(posts.get(index))
            onOpenMedia: Nav.openMedia(posts.get(index), 0)
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
            spacing: 14
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
