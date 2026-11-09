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
    required property string previewFallbackUrl
    required property int previewWidth
    required property int previewHeight
    required property int galleryCount

    property bool highlighted: false   // keyboard selection
    property bool saved: false
    property bool visited: false
    property bool compact: AppSettings.compactCards
    readonly property bool hasMedia: kind === PostListModel.ImagePost || kind === PostListModel.GalleryPost
                                     || kind === PostListModel.VideoPost

    signal openPost()
    signal toggleSave()
    signal openMedia()
    signal openSubreddit(string name)
    signal openLink(string link)

    readonly property bool hasBigMedia: !compact && previewUrl !== "" && hasMedia

    implicitHeight: body.implicitHeight + 2 * Theme.pad + shadowOffset

    Accessible.role: Accessible.ListItem
    Accessible.name: title + ", " + Api.community(subreddit) + ", " + score + " points, " + numComments + " comments"
    Accessible.onPressAction: openPost()
    readonly property int shadowOffset: 4
    readonly property int columnWidth: Math.min(width - 24, Theme.contentMaxWidth)

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
        // The card is as wide as the list; the face is the centred, readable column.
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: card.shadowOffset
        anchors.horizontalCenter: parent.horizontalCenter
        width: card.columnWidth
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
            border.width: card.highlighted ? 2 : 1
            border.color: card.highlighted ? Theme.accent
                        : cardHover.hovered ? Theme.alpha(Theme.accent, 0.6) : Theme.border
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
                        text: Api.community(card.subreddit)
                        color: Theme.text
                        font.pixelSize: Theme.fontMeta
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openSubreddit(card.subreddit) }
                    }
                    Text {
                        text: Api.user(card.author) + "  ·  " + Format.relativeTime(card.createdUtc)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                Icon { visible: card.stickied; name: "pin"; size: 18; color: Theme.stickied }
                Icon { visible: card.locked; name: "lock"; size: 18; color: Theme.caramel }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    text: card.title
                    color: card.visited && AppSettings.dimVisited ? Theme.textMuted : Theme.text
                    font.pixelSize: card.compact ? Theme.fontBody + 1 : Theme.fontTitle
                    font.weight: Font.Bold
                    wrapMode: Text.Wrap
                    lineHeight: 1.08
                    maximumLineCount: 6
                    elide: Text.ElideRight
                }
                // Compact mode: small thumbnail beside the title
                Item {
                    visible: card.compact && card.previewUrl !== "" && !card.nsfw && !card.spoiler
                    Layout.preferredWidth: 76
                    Layout.preferredHeight: 76
                    Layout.alignment: Qt.AlignTop
                    RoundedImage {
                        anchors.fill: parent
                        radius: 14
                        source: parent.visible ? card.previewUrl : ""
                        fallbackSource: parent.visible ? card.previewFallbackUrl : ""
                        showBroken: false
                    }
                    Rectangle {
                        visible: card.kind === PostListModel.VideoPost || card.kind === PostListModel.GalleryPost
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 4
                        width: 24
                        height: 24
                        radius: 12
                        color: "#B3000000"
                        Icon {
                            anchors.centerIn: parent
                            name: card.kind === PostListModel.VideoPost ? "play" : "gallery"
                            size: 14
                            color: "white"
                        }
                    }
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: card.hasMedia ? card.openMedia() : card.openLink(card.url)
                    }
                }
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
            PostMedia {
                visible: card.hasBigMedia
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? implicitHeight : 0
                kind: card.kind
                previewUrl: card.hasBigMedia ? card.previewUrl : ""
                previewFallbackUrl: card.hasBigMedia ? card.previewFallbackUrl : ""
                previewWidth: card.previewWidth
                previewHeight: card.previewHeight
                galleryCount: card.galleryCount
                spoiler: card.spoiler
                nsfw: card.nsfw
                onActivated: card.openMedia()
            }

            // External link preview
            Rectangle {
                visible: card.kind === PostListModel.LinkPost && !card.compact
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
                        showBroken: false
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
                maximumLineCount: card.compact ? 2 : 4
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
                    iconName: card.saved ? "bookmark-filled" : "bookmark"
                    size: 34
                    tooltip: card.saved ? qsTr("Unsave") : qsTr("Save")
                    tint: card.saved ? Theme.caramel : Theme.textMuted
                    fill: card.saved ? Theme.alpha(Theme.caramel, 0.16) : Theme.surfaceAlt
                    onClicked: card.toggleSave()
                }
                RoundButton2 {
                    iconName: "external"
                    size: 34
                    tooltip: qsTr("Open in browser")
                    tint: Theme.textMuted
                    onClicked: card.openLink(Api.webUrl(card.permalink))
                }
            }
        }
    }
}
