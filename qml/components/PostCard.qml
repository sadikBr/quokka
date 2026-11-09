import QtQuick
import QtQuick.Layouts

// One post in a feed. Bound to PostListModel roles.
Item {
    id: card

    required property int index
    required property string postId
    required property string title
    required property string author
    required property string subreddit
    required property string permalink
    required property string url
    required property string domain
    required property string selftext
    required property string flair
    required property string flairColor
    required property var createdUtc
    required property int score
    required property int numComments
    required property bool nsfw
    required property bool spoiler
    required property bool stickied
    required property bool locked
    required property int kind
    required property string previewUrl
    required property int previewWidth
    required property int previewHeight
    required property int galleryCount

    signal openPost()
    signal openSubreddit(string name)
    signal openLink(string link)

    readonly property bool hasBigMedia: previewUrl !== "" && (kind === PostListModel.ImagePost
                                        || kind === PostListModel.GalleryPost
                                        || kind === PostListModel.VideoPost)
    property bool spoilerRevealed: false

    implicitHeight: body.implicitHeight + 2 * Theme.pad + shadowOffset
    readonly property int shadowOffset: 4

    // Pop-in when the card first appears.
    opacity: 0
    transform: Translate { id: slide; y: 18 }
    Component.onCompleted: appear.start()
    ParallelAnimation {
        id: appear
        NumberAnimation { target: card; property: "opacity"; to: 1; duration: Theme.smooth; easing.type: Easing.OutCubic }
        NumberAnimation { target: slide; property: "y"; to: 0; duration: Theme.smooth + 80; easing.type: Easing.OutBack }
    }

    Item {
        id: face
        anchors.fill: parent
        anchors.bottomMargin: card.shadowOffset
        scale: cardTap.pressed ? 0.975 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.quick; easing.type: Easing.OutQuad } }

        // Solid offset shadow, sticker style.
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: card.shadowOffset
            anchors.bottomMargin: -card.shadowOffset
            radius: Theme.radiusCard
            color: Theme.shadow
        }
        Rectangle {
            id: bg
            anchors.fill: parent
            radius: Theme.radiusCard
            color: Theme.surface
            border.width: 1
            border.color: cardHover.hovered ? Theme.alpha(Theme.accent, 0.6) : Theme.border
            Behavior on border.color { ColorAnimation { duration: Theme.quick } }
        }

        HoverHandler { id: cardHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { id: cardTap; onTapped: card.openPost() }

        ColumnLayout {
            id: body
            anchors.fill: parent
            anchors.margins: Theme.pad
            spacing: 10

            // Header: subreddit, author, age
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Avatar {
                    name: card.subreddit
                    size: 34
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openSubreddit(card.subreddit) }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        text: "r/" + card.subreddit
                        color: Theme.text
                        font.pixelSize: Theme.fontMeta
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openSubreddit(card.subreddit) }
                    }
                    Text {
                        text: "u/" + card.author + "  ·  " + Format.relativeTime(card.createdUtc)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                Icon { visible: card.stickied; name: "pin"; size: 18; color: Theme.stickied }
                Icon { visible: card.locked; name: "lock"; size: 18; color: Theme.caramel }
            }

            Text {
                Layout.fillWidth: true
                text: card.title
                color: Theme.text
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
                wrapMode: Text.Wrap
                lineHeight: 1.08
                maximumLineCount: 6
                elide: Text.ElideRight
            }

            // Flair and content warnings
            Flow {
                Layout.fillWidth: true
                spacing: 6
                visible: card.flair !== "" || card.nsfw || card.spoiler
                Chip {
                    visible: card.flair !== ""
                    text: card.flair
                    fillColor: card.flairColor !== "" && card.flairColor !== "transparent"
                               ? card.flairColor : Theme.surfaceAlt
                    textColor: card.flairColor !== "" && card.flairColor !== "transparent"
                               ? Theme.readableOn(card.flairColor) : Theme.text
                    border.width: 0
                }
                Chip { visible: card.nsfw; text: "NSFW"; fillColor: Theme.nsfw; textColor: "white"; border.width: 0 }
                Chip { visible: card.spoiler; text: "Spoiler"; fillColor: Theme.spoiler; textColor: "white"; border.width: 0 }
            }

            // Big media: image, gallery, video
            Item {
                id: media
                visible: card.hasBigMedia
                Layout.fillWidth: true
                readonly property real ratio: card.previewWidth > 0 && card.previewHeight > 0
                                              ? card.previewHeight / card.previewWidth : 0.66
                Layout.preferredHeight: visible ? Math.min(560, Math.round(width * Math.max(0.45, Math.min(1.25, ratio)))) : 0

                RoundedImage {
                    anchors.fill: parent
                    source: card.hasBigMedia ? card.previewUrl : ""
                }

                // Spoiler cover
                Rectangle {
                    anchors.fill: parent
                    visible: card.spoiler && !card.spoilerRevealed
                    radius: Theme.radiusMedia
                    color: Theme.spoiler
                    Column {
                        anchors.centerIn: parent
                        spacing: 6
                        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "eye-off"; size: 30; color: "white" }
                        Text { text: "Spoiler, tap to reveal"; color: "white"; font.pixelSize: Theme.fontMeta; font.weight: Font.Bold }
                    }
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.spoilerRevealed = true }
                }

                // Gallery badge
                Chip {
                    visible: card.kind === PostListModel.GalleryPost
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 10
                    iconName: "gallery"
                    text: "1 / " + card.galleryCount
                    fillColor: "#B3000000"
                    textColor: "white"
                    border.width: 0
                }

                // Play button
                Rectangle {
                    visible: card.kind === PostListModel.VideoPost && !(card.spoiler && !card.spoilerRevealed)
                    anchors.centerIn: parent
                    width: 64
                    height: 64
                    radius: 32
                    color: Theme.alpha(Theme.accent, 0.92)
                    border.width: 3
                    border.color: "white"
                    Icon { anchors.centerIn: parent; anchors.horizontalCenterOffset: 3; name: "play"; size: 28; color: "white" }
                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        running: card.visible
                        NumberAnimation { to: 1.08; duration: 700; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
                    }
                }
            }

            // External link preview
            Rectangle {
                visible: card.kind === PostListModel.LinkPost
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 72 : 0
                radius: Theme.radiusMedia
                color: Theme.surfaceAlt

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 12
                    RoundedImage {
                        visible: card.previewUrl !== ""
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 56
                        radius: 12
                        source: card.kind === PostListModel.LinkPost ? card.previewUrl : ""
                        backgroundColor: Theme.surfaceAlt
                        placeholderColor: Theme.border
                    }
                    Rectangle {
                        visible: card.previewUrl === ""
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 56
                        radius: 12
                        color: Format.nameColor(card.domain)
                        Icon { anchors.centerIn: parent; name: "link"; size: 26; color: "white" }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            Layout.fillWidth: true
                            text: card.domain
                            color: Theme.text
                            font.pixelSize: Theme.fontMeta
                            font.weight: Font.Bold
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: card.url.replace(/^https?:\/\/(www\.)?/, "")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall
                            elide: Text.ElideRight
                        }
                    }
                    Icon { name: "external"; size: 18; color: Theme.textMuted }
                }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openLink(card.url) }
            }

            // Self-text excerpt
            Text {
                visible: card.kind === PostListModel.SelfPost && card.selftext !== ""
                Layout.fillWidth: true
                text: card.selftext
                textFormat: Text.PlainText
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
                wrapMode: Text.Wrap
                maximumLineCount: 4
                elide: Text.ElideRight
                lineHeight: 1.15
            }

            // Actions
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 2
                spacing: 8
                Chip {
                    iconName: "upvote"
                    text: Format.compact(card.score)
                    fontSize: Theme.fontMeta
                    fillColor: Theme.alpha(Theme.upvote, 0.14)
                    textColor: Theme.upvote
                    border.width: 0
                }
                Chip {
                    iconName: "comment"
                    text: Format.compact(card.numComments)
                    fontSize: Theme.fontMeta
                    fillColor: Theme.alpha(Theme.sky, 0.14)
                    textColor: Theme.sky
                    border.width: 0
                    clickable: true
                    onClicked: card.openPost()
                }
                Item { Layout.fillWidth: true }
                RoundButton2 {
                    iconName: "external"
                    size: 34
                    tooltip: qsTr("Open in browser")
                    tint: Theme.textMuted
                    onClicked: card.openLink(Format.webUrl(card.permalink))
                }
            }
        }
    }
}
