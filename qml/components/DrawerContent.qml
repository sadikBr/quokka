import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The contents of the side drawer / desktop sidebar.
Flickable {
    id: root
    signal navigate(var action)
    signal importRequested()

    contentHeight: col.implicitHeight + 40
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    readonly property string currentFeed: AppSettings.lastFeed.toLowerCase()

    ColumnLayout {
        id: col
        x: 16
        y: 20
        width: root.width - 32
        spacing: 6

        // Brand header
        RowLayout {
            spacing: 12
            Layout.bottomMargin: 10
            Image {
                source: Theme.logo
                sourceSize: Qt.size(56, 56)
                Layout.preferredWidth: 56
                Layout.preferredHeight: 56
            }
            ColumnLayout {
                spacing: 0
                Text {
                    text: "Quokka"
                    color: Theme.text
                    font.pixelSize: Theme.fontHuge
                    font.weight: Font.Black
                }
                Text {
                    text: qsTr("v%1 · a happy little Reddit reader").arg(Qt.application.version)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
            }
        }

        // ---- Navigation ----
        NavRow {
            iconName: "home"
            text: qsTr("Home")
            detail: Library.subscriptions.length > 0 ? String(Library.subscriptions.length) : ""
            accent: Theme.berry
            current: root.currentFeed === "home"
            onClicked: root.navigate(() => Nav.goToFeed("home"))
        }
        NavRow {
            iconName: "fire"
            text: qsTr("Popular")
            accent: Theme.caramel
            current: root.currentFeed === "popular"
            onClicked: root.navigate(() => Nav.goToFeed("popular"))
        }
        NavRow {
            iconName: "globe"
            text: qsTr("All")
            accent: Theme.mint
            current: root.currentFeed === "all"
            onClicked: root.navigate(() => Nav.goToFeed("all"))
        }
        NavRow {
            iconName: "search"
            text: qsTr("Search")
            accent: Theme.sky
            onClicked: root.navigate(() => Nav.openSearch("", ""))
        }
        NavRow {
            iconName: "bookmark"
            text: qsTr("Saved")
            detail: Library.savedCount > 0 ? String(Library.savedCount) : ""
            accent: Theme.caramel
            onClicked: root.navigate(() => Nav.openSaved())
        }
        NavRow {
            iconName: "history"
            text: qsTr("History")
            accent: Theme.grape
            onClicked: root.navigate(() => Nav.openHistory())
        }

        // ---- Communities ----
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 14
            SectionLabel { text: qsTr("My communities"); Layout.fillWidth: true }
            Chip {
                text: qsTr("Import")
                iconName: "upload"
                clickable: true
                onClicked: root.importRequested()
            }
        }
        Text {
            visible: Library.subscriptions.length === 0
            Layout.fillWidth: true
            text: qsTr("Tap Join on any community to add it to your Home feed, or import a list.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontMeta
            wrapMode: Text.Wrap
        }
        Repeater {
            model: Library.subscriptions
            delegate: NavRow {
                required property string modelData
                avatarName: modelData
                text: "r/" + modelData
                compact: true
                current: root.currentFeed === modelData.toLowerCase()
                onClicked: root.navigate(() => Nav.goToFeed(modelData))
            }
        }

        // ---- Settings ----
        SectionLabel { text: qsTr("Appearance"); Layout.topMargin: 14 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Repeater {
                model: [
                    { mode: AppSettings.System, label: qsTr("Auto"), icon: "system" },
                    { mode: AppSettings.Light, label: qsTr("Light"), icon: "sun" },
                    { mode: AppSettings.Dark, label: qsTr("Dark"), icon: "moon" }
                ]
                delegate: Chip {
                    required property var modelData
                    Layout.fillWidth: true
                    text: modelData.label
                    iconName: modelData.icon
                    fontSize: Theme.fontMeta
                    implicitHeight: 40
                    clickable: true
                    selected: AppSettings.themeMode === modelData.mode
                    onClicked: AppSettings.themeMode = modelData.mode
                }
            }
        }
        SettingSwitch {
            text: qsTr("Compact cards")
            detail: qsTr("Small thumbnails instead of big pictures.")
            checked: AppSettings.compactCards
            onToggled: (on) => AppSettings.compactCards = on
        }
        SettingSwitch {
            text: qsTr("Dim posts you've read")
            checked: AppSettings.dimVisited
            onToggled: (on) => AppSettings.dimVisited = on
        }

        SectionLabel { text: qsTr("Content"); Layout.topMargin: 14 }
        SettingSwitch {
            text: qsTr("Show NSFW posts")
            detail: qsTr("Hidden unless you turn this on. Media stays covered until tapped.")
            checked: AppSettings.showNsfw
            onToggled: (on) => AppSettings.showNsfw = on
        }
        SettingSwitch {
            text: qsTr("Start videos muted")
            checked: AppSettings.videoMuted
            onToggled: (on) => AppSettings.videoMuted = on
        }
        Text {
            text: qsTr("Default feed sort")
            color: Theme.text
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
            Layout.topMargin: 6
        }
        Flow {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: [
                    { key: "hot", label: qsTr("Hot") }, { key: "new", label: qsTr("New") },
                    { key: "top", label: qsTr("Top") }, { key: "rising", label: qsTr("Rising") }
                ]
                delegate: Chip {
                    required property var modelData
                    text: modelData.label
                    clickable: true
                    selected: AppSettings.defaultSort === modelData.key
                    onClicked: AppSettings.defaultSort = modelData.key
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            spacing: 8
            Chip {
                id: clearCache
                property bool done: false
                text: done ? qsTr("Cache cleared") : qsTr("Clear image cache")
                iconName: done ? "check" : "trash"
                clickable: !done
                onClicked: { Library.clearImageCache(); done = true }
            }
            Chip {
                text: qsTr("Clear history")
                iconName: "history"
                clickable: Library.historyCount > 0
                opacity: Library.historyCount > 0 ? 1 : 0.5
                onClicked: Library.clearHistory()
            }
        }

        // ---- About ----
        SectionLabel { text: qsTr("About"); Layout.topMargin: 14 }
        Text {
            Layout.fillWidth: true
            text: qsTr("Quokka is an open-source, read-only client that uses Reddit's public JSON. "
                       + "It is not affiliated with or endorsed by Reddit, Inc. "
                       + "Your subscriptions, saved posts and history stay on this device.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontMeta
            wrapMode: Text.Wrap
            lineHeight: 1.2
        }
        Flow {
            Layout.fillWidth: true
            spacing: 8
            Chip {
                text: qsTr("About & licenses")
                iconName: "text"
                clickable: true
                fontSize: Theme.fontMeta
                hpad: 14
                onClicked: root.navigate(() => Nav.openAbout())
            }
            Chip {
                text: qsTr("Shortcuts")
                iconName: "layout"
                clickable: true
                fontSize: Theme.fontMeta
                hpad: 14
                visible: Qt.platform.os !== "android"
                onClicked: root.navigate(() => Nav.shortcutsRequested())
            }
            Chip {
                text: qsTr("Source on GitHub")
                iconName: "github"
                clickable: true
                fontSize: Theme.fontMeta
                hpad: 14
                onClicked: Qt.openUrlExternally("https://github.com/sadikBr/Reddit-Client")
            }
        }
    }

    component SectionLabel: Text {
        color: Theme.accent
        font.pixelSize: Theme.fontSmall
        font.weight: Font.Black
        font.letterSpacing: 1.2
        font.capitalization: Font.AllUppercase
        Layout.topMargin: 4
        Layout.bottomMargin: 2
    }

    component NavRow: Rectangle {
        id: nav
        property string iconName
        property string avatarName
        property string text
        property string detail
        property color accent: Theme.accent
        property bool current: false
        property bool compact: false
        signal clicked()

        Layout.fillWidth: true
        implicitHeight: compact ? 40 : 46
        radius: height / 2
        color: current ? Theme.alpha(accent, 0.16) : navHover.hovered ? Theme.surfaceAlt : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 14
            spacing: 12
            Rectangle {
                visible: nav.iconName !== ""
                width: 30
                height: 30
                radius: 15
                color: Theme.alpha(nav.accent, 0.18)
                Icon { anchors.centerIn: parent; name: nav.iconName; size: 17; color: nav.accent }
            }
            Avatar {
                visible: nav.avatarName !== ""
                name: nav.avatarName
                size: 26
                backgroundColor: nav.color
            }
            Text {
                Layout.fillWidth: true
                text: nav.text
                color: Theme.text
                font.pixelSize: nav.compact ? Theme.fontMeta + 1 : Theme.fontBody
                font.weight: nav.current ? Font.Black : Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                visible: nav.detail !== ""
                text: nav.detail
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.Bold
            }
        }
        HoverHandler { id: navHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: nav.clicked() }
    }

    component SettingSwitch: Rectangle {
        id: sw
        property string text
        property string detail
        property bool checked
        signal toggled(bool on)

        Layout.fillWidth: true
        implicitHeight: swRow.implicitHeight + 20
        radius: Theme.radiusMedia
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        RowLayout {
            id: swRow
            anchors.fill: parent
            anchors.margins: 10
            anchors.leftMargin: 14
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: sw.text
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: sw.detail !== ""
                    text: sw.detail
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.Wrap
                }
            }
            Switch {
                checked: sw.checked
                onToggled: sw.toggled(checked)
            }
        }
    }
}
