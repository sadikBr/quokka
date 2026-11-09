import QtQuick
import QtQuick.Layouts

// One post in a feed: a flat row in the centred reading column, separated by a thin divider.
// Bound to PostListModel roles.
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
    readonly property bool covered: (nsfw && AppSettings.coverNsfw) || spoiler

    signal openPost()
    signal toggleSave()
    signal openMedia()
    signal openSubreddit(string name)
    signal openLink(string link)

    // YouTube links get a big poster with a play button; tapping opens YouTube.
    readonly property bool isYoutube: kind === PostListModel.LinkPost && previewUrl !== ""
                                      && /^https?:\/\/((www|m|music)\.)?(youtube\.com|youtu\.be)\//i.test(url)
    readonly property bool hasBigMedia: !compact && ((hasMedia && (previewUrl !== "" || kind !== PostListModel.ImagePost))
                                                     || isYoutube)
    // Small square picture beside the title: link previews, and media in compact mode.
    readonly property bool hasThumb: previewUrl !== "" && !covered && !hasBigMedia
                                     && (kind === PostListModel.LinkPost || (compact && hasMedia))
    readonly property int columnWidth: Math.min(width - 24, Theme.contentMaxWidth)
    readonly property int vpad: 14
    readonly property color rowColor: highlighted ? Theme.accentSoft : cardHover.hovered ? Theme.hover : Theme.bg

    implicitHeight: body.implicitHeight + 2 * vpad + 1

    Accessible.role: Accessible.ListItem
    Accessible.name: title + ", " + Api.community(subreddit) + ", " + score + " points, " + numComments + " comments"
    Accessible.onPressAction: openPost()

    Item {
        id: face
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: card.columnWidth

        // Hover / keyboard highlight, slightly wider than the text column.
        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: -10
            anchors.rightMargin: -10
            anchors.topMargin: 3
            anchors.bottomMargin: 4
            radius: Theme.radiusCard
            color: card.rowColor
            visible: card.highlighted || cardHover.hovered
        }

        HoverHandler { id: cardHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { id: cardTap; onTapped: card.openPost() }

        ColumnLayout {
            id: body
            y: card.vpad
            width: parent.width
            spacing: 8

            // Community · author · age
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Avatar {
                    name: card.subreddit
                    size: 20
                    backgroundColor: card.rowColor
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openSubreddit(card.subreddit) }
                }
                Text {
                    text: Api.community(card.subreddit)
                    color: Theme.text
                    font.pixelSize: Theme.fontSmall + 1
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    Layout.maximumWidth: face.width * 0.5
                    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openSubreddit(card.subreddit) }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
                Text {
                    Layout.fillWidth: true
                    text: "·  " + Api.shortName(card.author) + "  ·  " + Format.relativeTime(card.createdUtc)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    elide: Text.ElideRight
                }
                Icon { visible: card.stickied; name: "pin"; size: 15; color: Theme.stickied }
                Icon { visible: card.locked; name: "lock"; size: 15; color: Theme.caramel }
            }

            // Title, with a small thumbnail on the right for links (and media in compact mode)
            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    text: card.title
                    textFormat: Text.PlainText
                    color: card.visited && AppSettings.dimVisited ? Theme.textMuted : Theme.text
                    font.pixelSize: Theme.fontTitle + 1
                    font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                    lineHeight: 1.18
                    maximumLineCount: 5
                    elide: Text.ElideRight
                }
                Item {
                    visible: card.hasThumb
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    Layout.alignment: Qt.AlignTop
                    RoundedImage {
                        anchors.fill: parent
                        radius: Theme.radiusMedia
                        source: card.hasThumb ? card.previewUrl : ""
                        fallbackSource: card.hasThumb ? card.previewFallbackUrl : ""
                        showBroken: false
                        backgroundColor: card.rowColor
                    }
                    Rectangle {
                        visible: card.kind !== PostListModel.ImagePost
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 4
                        width: 22
                        height: 22
                        radius: 11
                        color: "#B3000000"
                        Icon {
                            anchors.centerIn: parent
                            name: card.kind === PostListModel.VideoPost ? "play"
                                : card.kind === PostListModel.GalleryPost ? "gallery" : "external"
                            size: 12
                            color: "white"
                        }
                    }
                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: card.hasMedia ? card.openMedia() : card.openLink(card.url)
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
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
                    readonly property bool colored: card.flairColor !== "" && card.flairColor !== "transparent"
                    fillColor: colored ? card.flairColor : Theme.surfaceAlt
                    textColor: colored ? Theme.readableOn(card.flairColor) : Theme.textMuted
                    fontSize: 11
                    hpad: 8
                }
                Chip { visible: card.nsfw; text: "NSFW"; fillColor: Theme.alpha(Theme.nsfw, 0.14); textColor: Theme.nsfw; fontSize: 11; hpad: 8 }
                Chip { visible: card.spoiler; text: qsTr("Spoiler"); fillColor: Theme.alpha(Theme.spoiler, 0.14); textColor: Theme.spoiler; fontSize: 11; hpad: 8 }
            }

            // Link domain
            Row {
                visible: card.kind === PostListModel.LinkPost && card.domain !== ""
                spacing: 5
                Icon { name: "link"; size: 14; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: card.domain
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    anchors.verticalCenter: parent.verticalCenter
                }
                TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: card.openLink(card.url) }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
            }

            // Big media: image, gallery, video
            PostMedia {
                visible: card.hasBigMedia
                Layout.fillWidth: true
                Layout.topMargin: 2
                Layout.preferredHeight: visible ? implicitHeight : 0
                kind: card.isYoutube ? PostListModel.VideoPost : card.kind
                previewUrl: card.hasBigMedia ? card.previewUrl : ""
                previewFallbackUrl: card.hasBigMedia ? card.previewFallbackUrl : ""
                previewWidth: card.previewWidth
                previewHeight: card.previewHeight
                galleryCount: card.galleryCount
                spoiler: card.spoiler
                nsfw: card.nsfw
                backgroundColor: card.rowColor
                onActivated: card.isYoutube ? card.openLink(card.url) : card.openMedia()
            }

            // Self-text excerpt
            Text {
                visible: card.kind === PostListModel.SelfPost && card.selftext !== ""
                Layout.fillWidth: true
                text: card.selftext
                textFormat: Text.PlainText
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody - 1
                wrapMode: Text.Wrap
                maximumLineCount: card.compact ? 2 : 3
                elide: Text.ElideRight
                lineHeight: 1.2
            }

            // Actions
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: -8
                spacing: 2
                ActionButton {
                    iconName: "upvote"
                    label: Format.compact(card.score)
                    tint: Theme.upvote
                }
                ActionButton {
                    iconName: "comment"
                    label: Format.compact(card.numComments)
                    onClicked: card.openPost()
                }
                Item { Layout.fillWidth: true }
                RoundButton2 {
                    iconName: card.saved ? "bookmark-filled" : "bookmark"
                    size: 34
                    tooltip: card.saved ? qsTr("Unsave") : qsTr("Save")
                    tint: card.saved ? Theme.accent : Theme.textMuted
                    onClicked: card.toggleSave()
                }
                RoundButton2 {
                    iconName: "external"
                    size: 34
                    tooltip: qsTr("Open in browser")
                    onClicked: card.openLink(Api.webUrl(card.permalink))
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.divider
        }
    }

    // Icon + number, quiet until hovered.
    component ActionButton: Rectangle {
        id: action
        property string iconName
        property string label
        property color tint: Theme.textMuted
        signal clicked()

        implicitHeight: 32
        implicitWidth: actionRow.implicitWidth + 16
        radius: height / 2
        color: actionHover.hovered ? Theme.alpha(Theme.text, 0.07) : "transparent"
        Row {
            id: actionRow
            anchors.centerIn: parent
            spacing: 6
            Icon { name: action.iconName; size: 16; color: action.tint; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: action.label
                color: Theme.textMuted
                font.pixelSize: Theme.fontMeta
                font.weight: Font.Medium
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        HoverHandler { id: actionHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: action.clicked() }
    }
}
