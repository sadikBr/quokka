import QtQuick
import QtQuick.Layouts

// A comment shown outside its thread (user profile), with the post it belongs to.
Item {
    id: card
    required property string bodyHtml
    required property int score
    required property var createdUtc
    required property string subreddit
    required property string linkTitle
    required property string permalink

    // The item spans the list; the visible box is a centred column.
    implicitHeight: box.implicitHeight

    Rectangle {
        id: box
        width: Math.min(card.width - 24, Theme.contentMaxWidth)
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
        implicitHeight: col.implicitHeight + 28
        radius: Theme.radiusCard
        color: hover.hovered ? Theme.hover : Theme.bg
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.divider
        }

        ColumnLayout {
            id: col
            anchors.fill: parent
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            anchors.topMargin: 14
            anchors.bottomMargin: 14
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Avatar { name: card.subreddit; size: 22; backgroundColor: box.color }
                Text {
                    Layout.fillWidth: true
                    text: Api.community(card.subreddit) + "  ·  " + Format.relativeTime(card.createdUtc)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Row {
                    spacing: 4
                    Icon { name: "upvote"; size: 14; color: Theme.upvote; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: Format.compact(card.score)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall + 1
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                text: card.linkTitle
                color: Theme.text
                font.pixelSize: Theme.fontMeta
                font.weight: Font.Bold
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
            RedditText {
                id: body
                Layout.fillWidth: true
                html: card.bodyHtml
            }
        }

        HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: Nav.openLink(Api.webUrl(card.permalink)) }
    }
}
