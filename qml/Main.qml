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
    // Behind the system bars (Android draws the app under them): the current page's colour.
    readonly property bool immersivePage: stack.currentItem !== null && stack.currentItem.immersive === true
    color: immersivePage ? "#0B0A0F" : Theme.bg
    Binding { target: SafeArea; property: "lightBars"; value: !Theme.dark && !window.immersivePage }
    onWidthChanged: SafeArea.refresh()
    onHeightChanged: SafeArea.refresh()

    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    Material.primary: Theme.accent
    Material.background: Theme.surface
    Material.foreground: Theme.text

    // Wide desktop windows get a permanent sidebar instead of the slide-out drawer.
    readonly property bool wide: width >= 1100 && Qt.platform.os !== "android"
    onWideChanged: { Nav.sidebarDocked = wide; if (wide) drawer.close() }

    Rectangle {
        id: sidebar
        visible: window.wide
        width: visible ? 272 : 0
        height: parent.height
        color: Theme.sidebar
        DrawerContent {
            anchors.fill: parent
            anchors.rightMargin: 1
            onNavigate: (action) => action()
            onImportRequested: importDialog.open()
        }
        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.border
        }
    }

    StackView {
        id: stack
        // Keep clear of the status bar, gesture bar and camera cut-outs (phones).
        anchors.top: parent.top
        anchors.topMargin: SafeArea.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: SafeArea.bottom
        anchors.left: sidebar.right
        anchors.leftMargin: SafeArea.left
        anchors.right: parent.right
        anchors.rightMargin: SafeArea.right
        clip: true
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

    Component {
        id: aboutPage
        AboutPage {}
    }

    Component {
        id: settingsPage
        SettingsPage {}
    }

    Component {
        id: mediaFeedPage
        MediaFeedPage {}
    }

    Component {
        id: sourcePage
        SourcePage {}
    }

    // `quokka <link>` opens that Lemmy (or Reddit) link on start.
    Component.onCompleted: {
        Nav.window = window
        Nav.sidebarDocked = wide
        restoreGeometry()
        const args = Qt.application.arguments
        for (let i = 1; i < args.length; ++i) {
            if (/^(https?:\/\/|\/?(r|c|u|user|post)\/)/.test(args[i])) {
                const link = args[i]
                Qt.callLater(() => Nav.openLink(link))
                break
            }
        }
    }

    // Switching servers: start over from a fresh main feed.
    Connections {
        target: Api
        function onChanged() {
            stack.clear()
            stack.push(rootFeed)
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
        function onAboutRequested() { stack.push(aboutPage) }
        function onShortcutsRequested() { shortcutsHelp.open() }
        function onSignInRequested() { signInDialog.open() }
        function onSettingsRequested() { stack.push(settingsPage) }
        function onMediaFeedRequested(params) { stack.push(mediaFeedPage, { params: params }) }
        function onSourceRequested(params) { stack.push(sourcePage, { params: params }) }
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
        interactive: !window.wide
        onImportRequested: { drawer.close(); importDialog.open() }
    }

    ImportDialog {
        id: importDialog
    }

    ShortcutsHelp {
        id: shortcutsHelp
    }

    SignInDialog {
        id: signInDialog
    }

    readonly property bool popupOpen: picker.opened || drawer.opened || importDialog.opened || shortcutsHelp.opened
                                      || signInDialog.opened

    // Back navigation: Esc / Backspace / Alt+Left on desktop, the system back button on Android.
    Shortcut {
        sequences: [StandardKey.Back, "Esc", "Backspace"]
        enabled: stack.depth > 1 && !window.popupOpen
        onActivated: stack.pop()
    }
    Shortcut {
        sequences: ["Ctrl+F"]
        enabled: !window.popupOpen
        onActivated: Nav.openSearch("", "")
    }
    Shortcut {
        sequences: ["Ctrl+L", "G"]
        enabled: !window.popupOpen
        onActivated: picker.open()
    }
    Shortcut {
        sequences: ["?", "F1"]
        enabled: !window.popupOpen
        onActivated: shortcutsHelp.open()
    }
    Shortcut {
        sequences: ["Home"]
        enabled: stack.depth > 1 && !window.popupOpen
        onActivated: stack.pop(null)
    }

    // Mouse "back" button
    TapHandler {
        acceptedButtons: Qt.BackButton
        onTapped: if (stack.depth > 1 && !window.popupOpen) stack.pop()
    }

    // Remember the window size and position on desktop.
    function restoreGeometry() {
        if (Qt.platform.os === "android")
            return
        const g = AppSettings.value("window/geometry", "")
        const p = String(g).split(",").map(Number)
        if (p.length === 4 && p.every(v => !isNaN(v)) && p[2] >= minimumWidth && p[3] >= minimumHeight) {
            x = p[0]; y = p[1]; width = p[2]; height = p[3]
        }
        if (AppSettings.value("window/maximized", false) === true || AppSettings.value("window/maximized", "") === "true")
            visibility = Window.Maximized
    }
    function saveGeometry() {
        if (Qt.platform.os === "android")
            return
        const maximized = visibility === Window.Maximized || visibility === Window.FullScreen
        AppSettings.setValue("window/maximized", maximized)
        if (!maximized)
            AppSettings.setValue("window/geometry", [x, y, width, height].join(","))
    }
    // On Android the system back button arrives as a close request: use it to step back.
    // On desktop, closing the window always quits.
    onClosing: (close) => {
        if (Qt.platform.os !== "android") {
            saveGeometry()
            return
        }
        if (drawer.opened) {
            drawer.close()
            close.accepted = false
        } else if (importDialog.opened) {
            importDialog.close()
            close.accepted = false
        } else if (signInDialog.opened) {
            signInDialog.close()
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
