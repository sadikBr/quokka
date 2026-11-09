import QtQuick
import QtQuick.Controls

// Saved posts or reading history (stored on this device).
Page {
    id: page
    property string source: "saved" // "saved" | "history"
    readonly property bool isHistory: source === "history"

    function back() { page.StackView.view.pop() }

    background: Rectangle { color: Theme.bg }

    LocalPostModel {
        id: localPosts
        source: page.source
    }

    header: TopBar {
        title: page.isHistory ? qsTr("History") : qsTr("Saved")
        subtitle: localPosts.count === 1 ? qsTr("1 post") : qsTr("%1 posts").arg(localPosts.count)
        onBack: page.back()
        RoundButton2 {
            visible: page.isHistory && localPosts.count > 0
            iconName: "trash"
            tooltip: qsTr("Clear history")
            onClicked: Library.clearHistory()
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        model: localPosts
        spacing: 0
        topMargin: 4
        bottomMargin: 24
        cacheBuffer: 1200
        readonly property int contentWidth2: Math.min(width - 24, Theme.contentMaxWidth)
        ScrollBar.vertical: ThinScrollBar {}

        delegate: PostCard {
            width: list.width
            view: list
            autoplay: page.StackView.status === StackView.Active && AppSettings.autoplayFeed
            saved: Library.savedRevision >= 0 && Library.isSaved(postId)
            visited: false
            onToggleSave: Library.toggleSaved(localPosts.get(index))
            onOpenPost: Nav.openPost(localPosts.get(index))
            onOpenMedia: (i) => Nav.openMedia(localPosts.get(index), i)
            onOpenSubreddit: (name) => Nav.openSubreddit(name)
            onOpenLink: (link) => Nav.openLink(link)
        }
    }
    WheelScroller {
        flickable: list
        enabled: list.visible
    }

    EmptyState {
        visible: localPosts.count === 0
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 420)
        title: page.isHistory ? qsTr("Nothing here yet") : qsTr("No saved posts")
        message: page.isHistory ? qsTr("Posts you open show up here.")
                                : qsTr("Tap the bookmark on any post to keep it here, even offline-ish.")
    }
}
