import QtQuick
import QtQuick.Layouts

// A comment shown outside its thread (user profile), with the post it belongs to.
Rectangle {
    id: card
    required property string bodyHtml
    required property int score
    required property var createdUtc
    required property string subreddit
    required property string linkTitle
    required property string permalink

    implicitHeight: col.implicitHeight + 28
    radius: Theme.radiusCard
    color: Theme.surface
    border.width: 1
    border.color: hover.hovered ? Theme.alpha(Theme.accent, 0.6) : Theme.border

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: 14
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Avatar { name: card.subreddit; size: 22; backgroundColor: card.color }
            Text {
                Layout.fillWidth: true
                text: Api.community(card.subreddit) + "  ·  " + Format.relativeTime(card.createdUtc)
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Chip {
                iconName: "upvote"
                text: Format.compact(card.score)
                fillColor: Theme.alpha(Theme.upvote, 0.14)
                textColor: Theme.upvote
                border.width: 0
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
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: body.implicitHeight + 16
            radius: 12
            color: Theme.surfaceAlt
            RedditText {
                id: body
                anchors.fill: parent
                anchors.margins: 8
                html: card.bodyHtml
            }
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: Nav.openLink(Api.webUrl(card.permalink)) }
}
