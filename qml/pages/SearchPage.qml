import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Search posts (everywhere or inside one community) and communities.
Page {
    id: page
    property string initialQuery
    property string subreddit          // scope; empty = everywhere
    property string query: initialQuery
    property int tab: 0                // 0 = posts, 1 = communities
    readonly property bool active: StackView.status === StackView.Active

    function back() { page.StackView.view.pop() }

    background: Rectangle { color: Theme.bg }
    Component.onCompleted: field.forceActiveFocus()

    PostListModel {
        id: results
        feed: page.subreddit !== "" ? page.subreddit : "all"
        searchQuery: page.query
        enabled: page.query !== "" && page.tab === 0
        sort: "relevance"
        timeRange: "all"
        showNsfw: AppSettings.showNsfw
    }
    SubredditListModel {
        id: communities
        query: page.tab === 1 ? page.query : ""
        showNsfw: AppSettings.showNsfw
    }

    Timer {
        id: debounce
        interval: 450
        onTriggered: page.query = field.text.trim()
    }

    header: Rectangle {
        implicitHeight: headerCol.implicitHeight + 20
        color: Theme.bg
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

        ColumnLayout {
            id: headerCol
            width: Math.min(parent.width - 24, Theme.contentMaxWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            y: 10
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                RoundButton2 {
                    iconName: "chevron-down"
                    rotation: 90
                    fill: Theme.alpha(Theme.accent, 0.14)
                    tint: Theme.accent
                    tooltip: qsTr("Back")
                    onClicked: page.back()
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 46
                    radius: 23
                    color: Theme.surfaceAlt
                    border.width: field.activeFocus ? 2 : 1
                    border.color: field.activeFocus ? Theme.accent : Theme.border
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 6
                        spacing: 6
                        Icon { name: "search"; size: 18; color: Theme.textMuted }
                        TextField {
                            id: field
                            Layout.fillWidth: true
                            text: page.initialQuery
                            placeholderText: page.subreddit !== "" ? qsTr("Search %1").arg(Api.community(page.subreddit))
                                                                  : qsTr("Search %1").arg(Api.serviceName)
                            placeholderTextColor: Theme.textMuted
                            color: Theme.text
                            font.pixelSize: Theme.fontBody
                            background: null
                            inputMethodHints: Qt.ImhNoPredictiveText
                            onTextEdited: debounce.restart()
                            onAccepted: { debounce.stop(); page.query = text.trim() }
                        }
                        RoundButton2 {
                            visible: field.text !== ""
                            iconName: "close"
                            size: 32
                            fill: "transparent"
                            tint: Theme.textMuted
                            tooltip: qsTr("Clear")
                            onClicked: { field.text = ""; page.query = ""; field.forceActiveFocus() }
                        }
                    }
                }
            }

            Chip {
                visible: page.subreddit !== ""
                text: qsTr("in %1 (tap to search everywhere)").arg(Api.community(page.subreddit))
                iconName: "close"
                clickable: true
                fontSize: Theme.fontSmall
                onClicked: page.subreddit = ""
            }

            // Lemmy's search uses the signed-in account's NSFW preference, and Quokka reads
            // anonymously, so servers never return NSFW results to a search.
            Text {
                visible: AppSettings.showNsfw && Api.isLemmy
                Layout.fillWidth: true
                text: qsTr("Lemmy servers leave NSFW posts and communities out of searches for readers "
                           + "who aren't signed in. Browse NSFW communities directly to see their posts.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.Wrap
            }

            SegmentedTabs {
                Layout.fillWidth: true
                tabs: [qsTr("Posts"), qsTr("Communities")]
                currentIndex: page.tab
                onPicked: (i) => page.tab = i
            }

            ListView {
                visible: page.tab === 0 && page.query !== ""
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 32 : 0
                orientation: ListView.Horizontal
                spacing: 6
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: [
                    { key: "relevance", label: qsTr("Relevant") }, { key: "top", label: qsTr("Top") },
                    { key: "new", label: qsTr("New") }, { key: "comments", label: qsTr("Most comments") }
                ]
                delegate: Chip {
                    required property var modelData
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                    text: modelData.label
                    clickable: true
                    hpad: 12
                    selected: results.sort === modelData.key
                    onClicked: results.sort = modelData.key
                }
            }
        }
    }

    // ---- Posts ----
    ListView {
        id: postList
        anchors.fill: parent
        visible: page.tab === 0
        model: results
        spacing: 14
        topMargin: 14
        bottomMargin: 24
        cacheBuffer: 1200
        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)
        ScrollBar.vertical: ScrollBar {}
        onContentYChanged: {
            if (results.count > 0 && !results.loading && results.hasMore
                    && contentY + height > originY + contentHeight - height * 1.5)
                results.loadMore()
        }
        delegate: PostCard {
            width: postList.width
            saved: Library.savedRevision >= 0 && Library.isSaved(postId)
            visited: Library.historyRevision >= 0 && Library.isVisited(postId)
            onToggleSave: Library.toggleSaved(results.get(index))
            onOpenPost: Nav.openPost(results.get(index))
            onOpenMedia: Nav.openMedia(results.get(index), 0)
            onOpenSubreddit: (name) => Nav.openSubreddit(name)
            onOpenLink: (link) => Nav.openLink(link)
        }
        footer: Item {
            width: postList.width
            height: 120
            LoadingDots { anchors.centerIn: parent; visible: results.loading }
            Text {
                anchors.centerIn: parent
                width: postList.contentWidth2
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: !results.loading && page.query !== "" && results.count === 0
                text: results.error !== "" ? results.error : qsTr("No posts found for “%1”.").arg(page.query)
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
            }
        }
    }

    EmptyState {
        visible: page.tab === 0 && page.query === ""
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 420)
        title: qsTr("What are you curious about?")
        message: qsTr("Type something above. Try “quokka”!")
    }

    // ---- Communities ----
    ListView {
        id: subList
        anchors.fill: parent
        visible: page.tab === 1
        model: communities
        spacing: 10
        topMargin: 14
        bottomMargin: 24
        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)
        ScrollBar.vertical: ScrollBar {}
        onContentYChanged: {
            if (communities.count > 0 && !communities.loading && communities.hasMore
                    && contentY + height > originY + contentHeight - height * 1.5)
                communities.loadMore()
        }
        header: Text {
            width: subList.width
            leftPadding: (subList.width - subList.contentWidth2) / 2
            bottomPadding: 10
            visible: page.query === ""
            height: visible ? implicitHeight : 0
            text: qsTr("Popular communities")
            color: Theme.accent
            font.pixelSize: Theme.fontSmall
            font.weight: Font.Black
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1.2
        }
        delegate: SubredditRow {
            id: subRow
            width: subList.width
            onClicked: Nav.openSubreddit(name)
            action: JoinButton { name: subRow.name }
        }
        footer: Item {
            width: subList.width
            height: 100
            LoadingDots { anchors.centerIn: parent; visible: communities.loading }
            Text {
                anchors.centerIn: parent
                visible: !communities.loading && communities.count === 0
                text: communities.error !== "" ? communities.error : qsTr("No communities found.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
            }
        }
    }
}
