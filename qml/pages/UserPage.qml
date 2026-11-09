import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A user's profile: avatar, karma, and their posts or comments.
Page {
    id: page
    property string userName
    property int tab: 0 // 0 = posts, 1 = comments
    readonly property bool active: StackView.status === StackView.Active

    function back() { page.StackView.view.pop() }

    background: Rectangle { color: Theme.bg }

    UserInfo {
        id: user
        name: page.userName
    }
    PostListModel {
        id: posts
        userName: page.userName
        sort: "new"
        showNsfw: AppSettings.showNsfw
        enabled: page.tab === 0 || posts.count > 0
    }
    UserCommentModel {
        id: userComments
        userName: page.userName
        enabled: page.tab === 1
    }

    Shortcut {
        sequences: [StandardKey.Refresh, "Ctrl+R"]
        enabled: page.active
        onActivated: page.tab === 0 ? posts.refresh() : userComments.refresh()
    }

    header: TopBar {
        title: "u/" + page.userName
        subtitle: user.loaded ? qsTr("%1 karma").arg(Format.compact(user.totalKarma)) : ""
        onBack: page.back()
        RoundButton2 {
            iconName: "external"
            tooltip: qsTr("Open in browser")
            fill: Theme.surfaceAlt
            onClicked: Qt.openUrlExternally("https://www.reddit.com/user/" + page.userName)
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: page.tab === 0 ? posts : userComments
        spacing: 14
        bottomMargin: 24
        cacheBuffer: 1200
        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)
        readonly property var current: page.tab === 0 ? posts : userComments
        ScrollBar.vertical: ScrollBar {}

        onContentYChanged: {
            if (current.count > 0 && !current.loading && current.hasMore
                    && contentY + height > originY + contentHeight - height * 1.5)
                current.loadMore()
        }

        header: Item {
            width: list.width
            height: profile.implicitHeight + 28
            ColumnLayout {
                id: profile
                width: list.contentWidth2
                anchors.horizontalCenter: parent.horizontalCenter
                y: 16
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: profileRow.implicitHeight + 32
                    radius: Theme.radiusCard
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Format.nameColor(page.userName) }
                        GradientStop { position: 1; color: Theme.caramel }
                    }
                    RowLayout {
                        id: profileRow
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 14
                        Rectangle {
                            width: 76
                            height: 76
                            radius: 38
                            color: "white"
                            Avatar {
                                anchors.centerIn: parent
                                name: page.userName
                                imageUrl: user.iconUrl
                                size: 68
                                backgroundColor: "white"
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                Layout.fillWidth: true
                                text: "u/" + page.userName
                                color: "white"
                                font.pixelSize: 24
                                font.weight: Font.Black
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: user.loaded
                                text: qsTr("Redditor for %1").arg(Format.age(user.createdUtc))
                                color: Theme.alpha("white", 0.9)
                                font.pixelSize: Theme.fontMeta
                                font.weight: Font.DemiBold
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: user.description !== ""
                                text: user.description
                                color: "white"
                                font.pixelSize: Theme.fontMeta
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                Text {
                    visible: user.error !== ""
                    Layout.fillWidth: true
                    text: user.error
                    color: Theme.nsfw
                    font.pixelSize: Theme.fontMeta
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    visible: user.loaded
                    Layout.fillWidth: true
                    spacing: 10
                    StatTile { label: qsTr("Post karma"); value: Format.compact(user.linkKarma); accent: Theme.upvote }
                    StatTile { label: qsTr("Comment karma"); value: Format.compact(user.commentKarma); accent: Theme.sky }
                    StatTile { label: qsTr("Total"); value: Format.compact(user.totalKarma); accent: Theme.mint }
                }

                SegmentedTabs {
                    Layout.fillWidth: true
                    tabs: [qsTr("Posts"), qsTr("Comments")]
                    currentIndex: page.tab
                    onPicked: (i) => page.tab = i
                }
            }
        }

        delegate: Loader {
            id: rowLoader
            required property var model
            required property int index
            width: list.contentWidth2
            x: (list.width - width) / 2
            sourceComponent: page.tab === 0 ? postDelegate : commentDelegate

            Component {
                id: postDelegate
                PostCard {
                    readonly property var m: rowLoader.model
                    index: rowLoader.index
                    postId: m.postId
                    title: m.title
                    author: m.author
                    subreddit: m.subreddit
                    permalink: m.permalink
                    url: m.url
                    domain: m.domain
                    selftext: m.selftext
                    flair: m.flair
                    flairColor: m.flairColor
                    createdUtc: m.createdUtc
                    score: m.score
                    numComments: m.numComments
                    nsfw: m.nsfw
                    spoiler: m.spoiler
                    stickied: m.stickied
                    locked: m.locked
                    kind: m.kind
                    previewUrl: m.previewUrl
                    previewWidth: m.previewWidth
                    previewHeight: m.previewHeight
                    galleryCount: m.galleryCount
                    saved: Library.savedRevision >= 0 && Library.isSaved(postId)
                    visited: Library.historyRevision >= 0 && Library.isVisited(postId)
                    onToggleSave: Library.toggleSaved(posts.get(rowLoader.index))
                    onOpenPost: Nav.openPost(posts.get(rowLoader.index))
                    onOpenMedia: Nav.openMedia(posts.get(rowLoader.index), 0)
                    onOpenSubreddit: (name) => Nav.openSubreddit(name)
                    onOpenLink: (link) => Nav.openLink(link)
                }
            }
            Component {
                id: commentDelegate
                UserCommentCard {
                    readonly property var m: rowLoader.model
                    bodyHtml: m.bodyHtml
                    score: m.score
                    createdUtc: m.createdUtc
                    subreddit: m.subreddit
                    linkTitle: m.linkTitle
                    permalink: m.permalink
                }
            }
        }

        footer: Item {
            width: list.width
            height: 100
            LoadingDots { anchors.centerIn: parent; visible: list.current.loading }
            Text {
                anchors.centerIn: parent
                visible: !list.current.loading && list.current.count === 0
                text: list.current.error !== "" ? list.current.error
                    : page.tab === 0 ? qsTr("No posts yet.") : qsTr("No comments yet.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
            }
        }
    }

    component StatTile: Rectangle {
        property string label
        property string value
        property color accent: Theme.accent
        Layout.fillWidth: true
        implicitHeight: 64
        radius: Theme.radiusMedia
        color: Theme.alpha(accent, 0.12)
        Column {
            anchors.centerIn: parent
            spacing: 2
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.value
                color: parent.parent.accent
                font.pixelSize: 20
                font.weight: Font.Black
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.parent.label
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
            }
        }
    }
}
