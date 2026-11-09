import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A post with its comment thread.
Page {
    id: page

    property string postId
    property string focusCommentId
    property var initialPost: ({})
    readonly property var post: comments.loaded ? comments.post : initialPost
    readonly property bool active: StackView.status === StackView.Active

    function back() { page.StackView.view.pop() }

    background: Rectangle { color: Theme.bg }

    CommentTreeModel {
        id: comments
        postId: page.postId
        focusCommentId: page.focusCommentId
        sort: AppSettings.commentSort
    }

    Shortcut {
        sequences: [StandardKey.Refresh, "Ctrl+R"]
        enabled: page.active
        onActivated: comments.refresh()
    }

    header: TopBar {
        title: page.post.subreddit ? "r/" + page.post.subreddit : qsTr("Post")
        subtitle: page.post.numComments !== undefined
                  ? (page.post.numComments === 1 ? qsTr("1 comment") : qsTr("%1 comments").arg(Format.compact(page.post.numComments)))
                  : ""
        avatarName: page.post.subreddit || ""
        onBack: page.back()
        onTitleClicked: if (page.post.subreddit) Nav.openSubreddit(page.post.subreddit)

        RoundButton2 {
            iconName: "refresh"
            tooltip: qsTr("Refresh")
            fill: Theme.surfaceAlt
            onClicked: comments.refresh()
        }
        RoundButton2 {
            iconName: "external"
            tooltip: qsTr("Open in browser")
            fill: Theme.surfaceAlt
            onClicked: Qt.openUrlExternally(Format.webUrl(page.post.permalink || ("/comments/" + page.postId)))
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: comments
        bottomMargin: 32
        cacheBuffer: 1600
        boundsBehavior: Flickable.StopAtBounds
        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)

        ScrollBar.vertical: ScrollBar {}

        header: Item {
            width: list.width
            height: postHeader.implicitHeight + 8
            PostHeader {
                id: postHeader
                width: list.contentWidth2
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        delegate: CommentRow {
            width: list.contentWidth2
            x: (list.width - width) / 2
            onToggle: comments.toggleCollapsed(index)
            onLoadMore: comments.loadMore(index)
            onContinueThread: Nav.openThread(page.postId, parentId.replace(/^t1_/, ""))
        }

        footer: Item {
            width: list.width
            height: footerContent.implicitHeight + 40
            Item {
                id: footerContent
                width: list.contentWidth2
                anchors.horizontalCenter: parent.horizontalCenter
                y: 20
                implicitHeight: comments.loading ? 40
                              : comments.error !== "" ? errorBox.implicitHeight
                              : comments.count === 0 ? emptyText.implicitHeight : 0
                LoadingDots {
                    anchors.centerIn: parent
                    visible: comments.loading
                }
                EmptyState {
                    id: errorBox
                    width: parent.width
                    visible: !comments.loading && comments.error !== ""
                    title: qsTr("Couldn't load comments")
                    message: comments.error
                    actionText: qsTr("Try again")
                    onAction: comments.refresh()
                }
                Text {
                    id: emptyText
                    width: parent.width
                    visible: !comments.loading && comments.error === "" && comments.loaded && comments.count === 0
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("No comments yet. Quiet in here!")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontBody
                }
            }
        }
    }

    component PostHeader: ColumnLayout {
        spacing: 12

        Item { Layout.preferredHeight: 4 }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Avatar {
                name: page.post.subreddit || "?"
                size: 38
                TapHandler { onTapped: Nav.openSubreddit(page.post.subreddit) }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    text: "r/" + (page.post.subreddit || "")
                    color: Theme.text
                    font.pixelSize: Theme.fontMeta
                    font.weight: Font.Bold
                    TapHandler { onTapped: Nav.openSubreddit(page.post.subreddit) }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
                Text {
                    text: "u/" + (page.post.author || "") + "  ·  " + Format.relativeTime(page.post.createdUtc || 0)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    TapHandler { onTapped: Nav.openUser(page.post.author) }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
            }
            Icon { visible: !!page.post.locked; name: "lock"; size: 18; color: Theme.caramel }
        }

        Text {
            Layout.fillWidth: true
            text: page.post.title || ""
            color: Theme.text
            font.pixelSize: 22
            font.weight: Font.Black
            wrapMode: Text.Wrap
            lineHeight: 1.05
            textFormat: Text.PlainText
        }

        Flow {
            Layout.fillWidth: true
            spacing: 6
            visible: !!page.post.flair || !!page.post.nsfw || !!page.post.spoiler
            Chip {
                visible: !!page.post.flair
                text: page.post.flair || ""
                readonly property bool colored: !!page.post.flairColor && page.post.flairColor !== "transparent"
                fillColor: colored ? page.post.flairColor : Theme.surfaceAlt
                textColor: colored ? Theme.readableOn(page.post.flairColor) : Theme.text
                border.width: 0
            }
            Chip { visible: !!page.post.nsfw; text: "NSFW"; fillColor: Theme.nsfw; textColor: "white"; border.width: 0 }
            Chip { visible: !!page.post.spoiler; text: "Spoiler"; fillColor: Theme.spoiler; textColor: "white"; border.width: 0 }
        }

        PostMedia {
            readonly property bool big: !!page.post.previewUrl && (page.post.kind === PostListModel.ImagePost
                                        || page.post.kind === PostListModel.GalleryPost
                                        || page.post.kind === PostListModel.VideoPost)
            visible: big
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            maxRatio: 1.6
            maxHeight: 680
            backgroundColor: Theme.bg
            kind: page.post.kind || 0
            previewUrl: big ? page.post.previewUrl : ""
            previewWidth: page.post.previewWidth || 0
            previewHeight: page.post.previewHeight || 0
            galleryCount: page.post.galleryCount || 0
            spoiler: !!page.post.spoiler
            nsfw: !!page.post.nsfw
            onActivated: Nav.openMedia(page.post, 0)
        }

        // Link card
        Rectangle {
            visible: page.post.kind === PostListModel.LinkPost
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 76 : 0
            radius: Theme.radiusMedia
            color: Theme.surfaceAlt
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12
                Rectangle {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    radius: 12
                    color: Format.nameColor(page.post.domain || "")
                    Icon { anchors.centerIn: parent; name: "link"; size: 26; color: "white" }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        Layout.fillWidth: true
                        text: page.post.domain || ""
                        color: Theme.text
                        font.pixelSize: Theme.fontMeta
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (page.post.url || "").replace(/^https?:\/\/(www\.)?/, "")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                    }
                }
                Icon { name: "external"; size: 18; color: Theme.textMuted }
            }
            TapHandler { onTapped: Nav.openLink(page.post.url) }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
        }

        RedditText {
            Layout.fillWidth: true
            visible: html !== ""
            html: page.post.selftextHtml || ""
            font.pixelSize: Theme.fontBody + 1
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Chip {
                iconName: "upvote"
                text: Format.compact(page.post.score || 0)
                fontSize: Theme.fontMeta
                fillColor: Theme.alpha(Theme.upvote, 0.14)
                textColor: Theme.upvote
                border.width: 0
            }
            Chip {
                visible: (page.post.upvoteRatio || 0) > 0
                text: Math.round((page.post.upvoteRatio || 0) * 100) + "% upvoted"
                fontSize: Theme.fontMeta
                fillColor: Theme.alpha(Theme.mint, 0.14)
                textColor: Theme.mint
                border.width: 0
            }
            Chip {
                iconName: "comment"
                text: Format.compact(page.post.numComments || 0)
                fontSize: Theme.fontMeta
                fillColor: Theme.alpha(Theme.sky, 0.14)
                textColor: Theme.sky
                border.width: 0
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 1
            color: Theme.border
        }

        // Single-thread banner
        Rectangle {
            visible: page.focusCommentId !== ""
            Layout.fillWidth: true
            implicitHeight: focusRow.implicitHeight + 20
            radius: Theme.radiusMedia
            color: Theme.alpha(Theme.sky, 0.12)
            RowLayout {
                id: focusRow
                anchors.fill: parent
                anchors.margins: 10
                Text {
                    Layout.fillWidth: true
                    text: qsTr("You're viewing a single thread.")
                    color: Theme.text
                    font.pixelSize: Theme.fontMeta
                    wrapMode: Text.Wrap
                }
                Chip {
                    text: qsTr("View all comments")
                    clickable: true
                    selected: true
                    selectedColor: Theme.sky
                    onClicked: page.focusCommentId = ""
                }
            }
        }

        // Comment sort
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            orientation: ListView.Horizontal
            spacing: 6
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: [
                { key: "confidence", label: qsTr("Best") }, { key: "top", label: qsTr("Top") },
                { key: "new", label: qsTr("New") }, { key: "controversial", label: qsTr("Spicy") },
                { key: "old", label: qsTr("Old") }, { key: "qa", label: qsTr("Q&A") }
            ]
            delegate: Chip {
                required property var modelData
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                text: modelData.label
                fontSize: Theme.fontMeta
                hpad: 12
                clickable: true
                selected: comments.sort === modelData.key
                selectedColor: Theme.sky
                onClicked: comments.sort = modelData.key
            }
        }
    }
}
