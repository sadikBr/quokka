import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A media site (Redgifs, Eporner) as a feed: sort tabs, search, tags from what's showing, and
// for Redgifs creator pages. The swipe button opens the same list full screen.
Page {
    id: page
    // source: "redgifs" | "eporner"; optionally user (a Redgifs creator), tag, search
    property var params: ({})
    readonly property string source: params.source || "redgifs"
    readonly property string sourceName: source === "eporner" ? "Eporner" : "Redgifs"
    readonly property string user: params.user || ""
    readonly property bool isRedgifs: source === "redgifs"
    // Redgifs: exact tag names, all required (tags are how Redgifs searches). Eporner: free text.
    property var tags: params.tag ? [params.tag] : []
    property string query: isRedgifs ? "" : (params.tag || params.search || "")
    function addTag(t) {
        t = String(t).trim()
        if (t === "" || tags.some(x => x.toLowerCase() === t.toLowerCase()))
            return
        tags = tags.concat([t])
        list.positionViewAtBeginning()
    }
    function removeTag(t) { tags = tags.filter(x => x !== t) }
    readonly property string searchText: isRedgifs ? tags.join("+") : query
    property int sortIndex: 0
    readonly property var sorts: ["hot", "new", "top"]
    readonly property var sortNames: source === "eporner" ? [qsTr("Popular"), qsTr("New"), qsTr("Top")]
                                                          : [qsTr("Trending"), qsTr("New"), qsTr("Top")]
    readonly property bool active: StackView.status === StackView.Active
    readonly property string heading: user !== "" ? "@" + user
                                    : tags.length > 0 ? tags.join(" + ")
                                    : query !== "" ? query : sourceName

    function back() { page.StackView.view.pop() }

    background: Rectangle { color: Theme.bg }

    PostListModel {
        id: posts
        source: page.source
        feed: "all"
        userName: page.user
        searchQuery: page.searchText
        sort: page.sorts[page.sortIndex]
        timeRange: "week"
        showNsfw: true
        hideBroken: AppSettings.hideBrokenMedia
    }

    // Tags of what's showing, most common first: tap one to browse it.
    property var tagChips: []
    function collectTags() {
        const counts = {}
        for (let i = 0; i < Math.min(posts.count, 60); ++i)
            for (const t of (posts.get(i).tags || []))
                counts[t] = (counts[t] || 0) + 1
        tagChips = Object.keys(counts).sort((a, b) => counts[b] - counts[a])
                         .filter(t => !page.tags.some(x => x.toLowerCase() === t.toLowerCase())
                                      && t.toLowerCase() !== page.query.toLowerCase()).slice(0, 14)
    }
    Connections {
        target: posts
        function onCountChanged() { Qt.callLater(page.collectTags) }
    }

    RedgifsSearch { id: rg }

    Shortcut {
        sequences: [StandardKey.Refresh, "Ctrl+R"]
        enabled: page.active
        onActivated: posts.refresh()
    }

    header: Rectangle {
        implicitHeight: headerCol.implicitHeight + 12
        color: Theme.bg
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }
        ColumnLayout {
            id: headerCol
            width: Math.min(parent.width - 24, Theme.contentMaxWidth + 16)
            anchors.horizontalCenter: parent.horizontalCenter
            y: 6
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                RoundButton2 {
                    iconName: "chevron-left"
                    tint: Theme.text
                    tooltip: qsTr("Back")
                    onClicked: page.back()
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        Layout.fillWidth: true
                        text: page.heading
                        color: Theme.text
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }
                    Text {
                        visible: page.user !== "" || page.query !== "" || page.tags.length > 0
                        text: page.sourceName
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSmall
                    }
                }
                RoundButton2 {
                    iconName: "reels"
                    tint: Theme.text
                    tooltip: qsTr("Swipe through them")
                    onClicked: Nav.openMediaFeed({ source: page.source, feed: "all", userName: page.user,
                                                   searchQuery: page.searchText, sort: posts.sort, timeRange: posts.timeRange,
                                                   title: page.heading })
                }
                RoundButton2 {
                    iconName: "refresh"
                    tint: Theme.text
                    tooltip: qsTr("Refresh")
                    onClicked: posts.refresh()
                }
            }

            // Search (not on a creator's page: their clips are listed as they are)
            Rectangle {
                visible: page.user === ""
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: 12
                color: Theme.surfaceAlt
                border.width: field.activeFocus ? 1 : 0
                border.color: Theme.accent
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 6
                    spacing: 6
                    Icon { name: "search"; size: 18; color: Theme.textMuted }
                    PlainField {
                        id: field
                        Layout.fillWidth: true
                        text: page.query
                        placeholderText: page.isRedgifs ? (page.tags.length > 0 ? qsTr("Add a tag…") : qsTr("Search tags or creators"))
                                                        : qsTr("Search %1").arg(page.sourceName)
                        placeholderTextColor: Theme.textMuted
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        inputMethodHints: Qt.ImhNoPredictiveText
                        onTextEdited: if (page.isRedgifs) rg.query = text
                        onAccepted: {
                            if (page.isRedgifs) {
                                // Free text: the best matching Redgifs tag.
                                if (text.trim() !== "")
                                    page.addTag(rg.bestTag(text))
                                text = ""
                                rg.query = ""
                            } else {
                                page.query = text.trim()
                            }
                        }
                    }
                    RoundButton2 {
                        visible: field.text !== ""
                        iconName: "close"
                        size: 30
                        fill: "transparent"
                        tint: Theme.textMuted
                        tooltip: qsTr("Clear")
                        onClicked: { field.text = ""; page.query = ""; rg.query = "" }
                    }
                }
            }

            // Redgifs: suggestions while typing (tags with clip counts, creators).
            Rectangle {
                visible: page.isRedgifs && field.text.trim().length >= 2 && rg.suggestions.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? suggestCol.implicitHeight + 8 : 0
                radius: 12
                color: Theme.surface
                border.width: 1
                border.color: Theme.border
                Column {
                    id: suggestCol
                    x: 4
                    y: 4
                    width: parent.width - 8
                    Repeater {
                        model: rg.suggestions.slice(0, 8)
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool isUser: modelData.type === "user" || modelData.type === "creator"
                            width: suggestCol.width
                            height: 40
                            radius: 8
                            color: sugHover.hovered ? Theme.hover : "transparent"
                            Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 10
                                Icon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    name: parent.parent.isUser ? "user" : "search"
                                    size: 16
                                    color: Theme.textMuted
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: (parent.parent.isUser ? "@" : "") + parent.parent.modelData.text
                                    color: Theme.text
                                    font.pixelSize: Theme.fontBody
                                }
                            }
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                visible: parent.modelData.count > 0
                                text: qsTr("%1 clips").arg(Format.compact(parent.modelData.count))
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                            }
                            HoverHandler { id: sugHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onTapped: {
                                    if (parent.isUser)
                                        Nav.openSource({ source: "redgifs", user: parent.modelData.text })
                                    else
                                        page.addTag(parent.modelData.text)
                                    field.text = ""
                                    rg.query = ""
                                }
                            }
                        }
                    }
                }
            }

            // Redgifs: the tags being searched (all required); tap × to drop one.
            Flow {
                visible: page.isRedgifs && page.tags.length > 0
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: page.tags
                    Chip {
                        required property string modelData
                        text: modelData
                        iconName: "close"
                        selected: true
                        fontSize: 12
                        clickable: true
                        onClicked: page.removeTag(modelData)
                    }
                }
            }

            SegmentedTabs {
                Layout.fillWidth: true
                tabs: page.sortNames
                currentIndex: page.sortIndex
                onPicked: (i) => page.sortIndex = i
            }

            // Tags to try: Redgifs' trending tags when nothing is picked, else tags from the
            // results (tap to narrow down). Eporner: keywords of what's showing.
            Text {
                visible: chipRepeater.count > 0 && page.user === ""
                text: page.isRedgifs && page.tags.length === 0 ? qsTr("Trending tags") : qsTr("Related")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
            }
            Flow {
                Layout.fillWidth: true
                spacing: 6
                visible: chipRepeater.count > 0 && page.user === ""
                Repeater {
                    id: chipRepeater
                    model: page.isRedgifs && page.tags.length === 0 && rg.trending.length > 0
                           ? rg.trending.slice(0, 18).map(t => t.text) : page.tagChips
                    Chip {
                        required property string modelData
                        text: "#" + modelData
                        fontSize: 12
                        clickable: true
                        onClicked: {
                            if (page.isRedgifs)
                                page.addTag(modelData)
                            else {
                                page.query = modelData
                                list.positionViewAtBeginning()
                            }
                        }
                    }
                }
            }
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: posts
        cacheBuffer: 1200
        bottomMargin: 24
        ScrollBar.vertical: ThinScrollBar {}
        onContentYChanged: {
            if (posts.count > 0 && !posts.loading && posts.hasMore
                    && contentY + height > originY + contentHeight - height * 1.5)
                posts.loadMore()
        }
        delegate: PostCard {
            width: list.width
            view: list
            autoplay: page.active && AppSettings.autoplayFeed
            saved: Library.savedRevision >= 0 && Library.isSaved(postId)
            visited: false
            onToggleSave: Library.toggleSaved(posts.get(index))
            onOpenPost: Nav.openPost(posts.get(index))
            onOpenMedia: (i) => Nav.openMedia(posts.get(index), i)
            onOpenSubreddit: (name) => Nav.openSubreddit(name)
            onOpenLink: (link) => Nav.openLink(link)
        }
        footer: Item {
            width: list.width
            height: 100
            LoadingDots { anchors.centerIn: parent; visible: posts.loading }
            Text {
                anchors.centerIn: parent
                width: parent.width - 48
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: !posts.loading && posts.count === 0
                text: posts.error !== "" ? posts.error : qsTr("Nothing here.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
            }
        }
    }
    WheelScroller { flickable: list }
}
