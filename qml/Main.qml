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
            onSubredditRequested: (name) => window.openSubreddit(name)
        }
    }

    Component {
        id: subFeed
        FeedPage {
            isRoot: false
            onBackRequested: stack.pop()
            onPickerRequested: picker.open()
            onSubredditRequested: (name) => {
                if (name.toLowerCase() !== initialFeed.toLowerCase())
                    window.openSubreddit(name)
            }
        }
    }

    function openSubreddit(name) {
        stack.push(subFeed, { initialFeed: name })
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
        } else if (picker.opened) {
            picker.close()
            close.accepted = false
        } else if (stack.depth > 1) {
            stack.pop()
            close.accepted = false
        }
    }
}
