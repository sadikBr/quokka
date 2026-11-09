import QtQuick
import QtQuick.Layouts

// One row of a comment thread: a comment, a "load more" button, or a "continue thread" link.
Item {
    id: row

    required property int index
    required property int kind
    required property string commentId
    required property string fullname
    required property string parentId
    required property int depth
    required property string author
    required property string bodyHtml
    required property string flair
    required property string distinguished
    required property int score
    required property bool scoreHidden
    required property var createdUtc
    required property bool isSubmitter
    required property bool stickied
    required property bool edited
    required property bool collapsed
    required property int hiddenCount
    required property int moreCount
    required property bool loadingMore

    signal toggle()
    signal loadMore()
    signal continueThread()

    readonly property int step: 14
    readonly property int maxIndentLevels: 8
    readonly property int indent: Math.min(depth, maxIndentLevels) * step
    readonly property var railColors: [Theme.berry, Theme.caramel, Theme.mint, Theme.sky, Theme.grape, "#FF6CCB"]

    implicitHeight: (kind === CommentTreeModel.CommentRow ? commentCol.implicitHeight + 18
                                                         : 44) + (depth === 0 ? 10 : 0)

    // Thread rails for each ancestor level
    Repeater {
        model: Math.min(row.depth, row.maxIndentLevels)
        delegate: Rectangle {
            required property int index
            x: index * row.step + 5
            y: 0
            width: 2
            height: row.height
            radius: 1
            color: row.railColors[index % row.railColors.length]
            opacity: 0.35
        }
    }

    // ---- Comment ----
    ColumnLayout {
        id: commentCol
        visible: row.kind === CommentTreeModel.CommentRow
        x: row.indent + (row.depth > 0 ? 8 : 0)
        y: row.depth === 0 ? 14 : 4
        width: row.width - x
        spacing: 4
        opacity: row.collapsed ? 0.75 : 1

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Avatar {
                name: row.author
                size: 22
            }
            Text {
                text: Api.shortName(row.author)
                color: row.isSubmitter ? Theme.accent
                     : row.distinguished === "moderator" ? Theme.mint
                     : row.distinguished === "admin" ? Theme.nsfw : Theme.text
                font.pixelSize: Theme.fontMeta
                font.weight: Font.Bold
                elide: Text.ElideRight
                Layout.maximumWidth: 160
                TapHandler {
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onTapped: Nav.openUser(row.author)
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
            }
            Chip {
                visible: row.isSubmitter
                text: "OP"
                fontSize: 10
                hpad: 6
                fillColor: Theme.accent
                textColor: "white"
                border.width: 0
            }
            Chip {
                visible: row.distinguished === "moderator" || row.distinguished === "admin"
                text: row.distinguished === "admin" ? "ADMIN" : "MOD"
                fontSize: 10
                hpad: 6
                fillColor: row.distinguished === "admin" ? Theme.nsfw : Theme.mint
                textColor: "white"
                border.width: 0
            }
            Icon { visible: row.stickied; name: "pin"; size: 14; color: Theme.stickied }
            Text {
                Layout.fillWidth: true
                text: "· " + Format.relativeTime(row.createdUtc) + (row.edited ? " · edited" : "")
                      + (row.flair !== "" ? " · " + row.flair : "")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
            Chip {
                visible: row.collapsed && row.hiddenCount > 0
                text: "+" + row.hiddenCount
                fontSize: 11
                hpad: 8
                fillColor: Theme.alpha(Theme.sky, 0.18)
                textColor: Theme.sky
                border.width: 0
            }
            Row {
                spacing: 3
                Icon {
                    name: "upvote"
                    size: 14
                    color: Theme.upvote
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: row.scoreHidden ? "•" : Format.compact(row.score)
                    color: Theme.upvote
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        RedditText {
            id: body
            visible: !row.collapsed
            Layout.fillWidth: true
            html: row.bodyHtml
        }
    }

    // Tap anywhere on a comment (outside links) to collapse / expand it.
    TapHandler {
        enabled: row.kind === CommentTreeModel.CommentRow
        onTapped: (eventPoint) => {
            // Let links and hidden spoilers in the body handle their own taps.
            const p = row.mapToItem(body, eventPoint.position.x, eventPoint.position.y)
            const inBody = body.visible && p.x >= 0 && p.y >= 0 && p.x <= body.width && p.y <= body.height
            if (inBody && (body.linkAt(p.x, p.y) !== "" || (body.hasSpoilers && !body.spoilersRevealed)))
                return
            row.toggle()
        }
    }

    // ---- Load more / continue thread ----
    Chip {
        visible: row.kind !== CommentTreeModel.CommentRow
        x: row.indent + (row.depth > 0 ? 8 : 0)
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: row.depth === 0 ? 5 : 0
        clickable: !row.loadingMore
        fontSize: Theme.fontMeta
        hpad: 14
        fillColor: Theme.alpha(Theme.sky, 0.14)
        textColor: Theme.sky
        border.width: 0
        iconName: row.kind === CommentTreeModel.ContinueRow ? "comment" : "chevron-down"
        text: row.loadingMore ? qsTr("Loading…")
            : row.kind === CommentTreeModel.ContinueRow ? qsTr("Continue this thread")
            : row.depth === 0 ? qsTr("Load %1 more comments").arg(row.moreCount)
            : row.moreCount === 1 ? qsTr("1 more reply")
            : qsTr("%1 more replies").arg(row.moreCount)
        onClicked: row.kind === CommentTreeModel.ContinueRow ? row.continueThread() : row.loadMore()
    }
}
