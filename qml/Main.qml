import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 1000
    height: 820
    minimumWidth: 340
    minimumHeight: 480
    visible: true
    title: "Quokka"
    color: Theme.bg

    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    Material.primary: Theme.accent
    Material.background: Theme.surface
    Material.foreground: Theme.text

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: rootFeed

        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: stack.width * 0.3; to: 0; duration: Theme.smooth; easing.type: Easing.OutCubic }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.smooth }
            }
        }
        pushExit: Transition {
            NumberAnimation { property: "opacity"; to: 0.4; duration: Theme.smooth }
        }
        popEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0.4; to: 1; duration: Theme.smooth }
        }
        popExit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; to: stack.width * 0.3; duration: Theme.smooth; easing.type: Easing.InCubic }
                NumberAnimation { property: "opacity"; to: 0; duration: Theme.smooth }
            }
        }
    }

    Component {
        id: rootFeed
        FeedPage {
            isRoot: true
            onMenuRequested: drawer.open()
            onPickerRequested: picker.open()
        }
    }

    Component {
        id: subFeed
        FeedPage {
            isRoot: false
            onBackRequested: stack.pop()
            onPickerRequested: picker.open()
        }
    }

    Component {
        id: postPage
        PostPage {}
    }

    Component {
        id: mediaViewer
        MediaViewer {}
    }

    Component {
        id: userPage
        UserPage {}
    }

    Component {
        id: searchPage
        SearchPage {}
    }

    Component {
        id: localPostsPage
        LocalPostsPage {}
    }

    // `quokka <reddit link>` opens that link on start.
    Component.onCompleted: {
        Nav.window = window
        const args = Qt.application.arguments
        for (let i = 1; i < args.length; ++i) {
            if (/^(https?:\/\/|\/?(r|u|user)\/)/.test(args[i])) {
                const link = args[i]
                Qt.callLater(() => Nav.openLink(link))
                break
            }
        }
    }

    // Page routing
    Connections {
        target: Nav
        function onSubredditRequested(name) { stack.push(subFeed, { initialFeed: name }) }
        function onPostRequested(post) {
            stack.push(postPage, { postId: post.postId, initialPost: post })
        }
        function onThreadRequested(postId, commentId) {
            stack.push(postPage, { postId: postId, focusCommentId: commentId })
        }
        function onMediaRequested(post, index) {
            const fromPost = stack.currentItem && stack.currentItem.postId === post.postId
            stack.push(mediaViewer, { post: post, startIndex: index, showCommentsButton: !fromPost })
        }
        function onImageRequested(url) {
            stack.push(mediaViewer, { post: { media: [{ url: url, caption: "", animated: /\.gif/i.test(url) }] },
                                      showCommentsButton: false })
        }
        function onUserRequested(name) { stack.push(userPage, { userName: name }) }
        function onSavedRequested() { stack.push(localPostsPage, { source: "saved" }) }
        function onHistoryRequested() { stack.push(localPostsPage, { source: "history" }) }
        function onRootFeedRequested(feed) {
            stack.pop(null)
            stack.currentItem.goTo(feed)
        }
        function onSearchRequested(query, subreddit) {
            stack.push(searchPage, { initialQuery: query, subreddit: subreddit })
        }
    }

    // Picking a community from the sheet changes the current page's feed.
    function goTo(feed) {
        stack.currentItem.goTo(feed)
    }

    SubredditPicker {
        id: picker
        onPicked: (feed) => window.goTo(feed)
    }

    SettingsDrawer {
        id: drawer
        height: window.height
        onImportRequested: { drawer.close(); importDialog.open() }
    }

    ImportDialog {
        id: importDialog
    }

    // Back navigation: Esc / Backspace on desktop, the system back button on Android.
    Shortcut {
        sequences: [StandardKey.Back, "Esc", "Backspace"]
        enabled: stack.depth > 1 && !picker.opened && !drawer.opened
        onActivated: stack.pop()
    }
    // On Android the system back button arrives as a close request: use it to step back.
    // On desktop, closing the window always quits.
    onClosing: (close) => {
        if (Qt.platform.os !== "android")
            return
        if (drawer.opened) {
            drawer.close()
            close.accepted = false
        } else if (importDialog.opened) {
            importDialog.close()
            close.accepted = false
        } else if (picker.opened) {
            picker.close()
            close.accepted = false
        } else if (stack.depth > 1) {
            stack.pop()
            close.accepted = false
        }
    }
}
