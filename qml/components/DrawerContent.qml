import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Navigation: account, feeds, your communities. Settings live on their own page.
Flickable {
    id: root
    signal navigate(var action)
    signal importRequested()

    contentHeight: col.implicitHeight + 24
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ThinScrollBar {}

    readonly property string currentFeed: AppSettings.lastFeed.toLowerCase()
    // Signed in: the account's subscriptions; otherwise the communities joined in Quokka.
    readonly property var communities: Account.signedIn ? Account.follows : Library.subscriptions

    ColumnLayout {
        id: col
        x: 12
        y: 16
        width: root.width - 24
        spacing: 2

        // Brand
        RowLayout {
            Layout.leftMargin: 6
            Layout.bottomMargin: 14
            spacing: 10
            Image {
                source: Theme.logo
                sourceSize: Qt.size(34, 34)
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
            }
            ColumnLayout {
                spacing: 0
                Text {
                    text: "Quokka"
                    color: Theme.text
                    font.pixelSize: 18
                    font.weight: Font.Bold
                }
                Text {
                    text: Api.serviceName
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
            }
        }

        // Account
        NavRow {
            visible: Account.supported && !Account.signedIn
            iconName: "user"
            text: qsTr("Sign in")
            detail: Api.instance
            onClicked: root.navigate(() => Nav.openSignIn())
        }
        NavRow {
            visible: Account.signedIn
            avatarName: Account.name || "?"
            avatarUrl: Account.avatarUrl
            text: Account.name !== "" ? Api.user(Account.name) : qsTr("Signed in")
            onClicked: root.navigate(() => Nav.openUser(Account.name))
        }
        Divider {}

        NavRow {
            iconName: "home"
            text: qsTr("Home")
            current: root.currentFeed === "home"
            onClicked: root.navigate(() => Nav.goToFeed("home"))
        }
        NavRow {
            iconName: "globe"
            text: qsTr("All")
            current: root.currentFeed === "all" || root.currentFeed === "popular"
            onClicked: root.navigate(() => Nav.goToFeed("all"))
        }
        NavRow {
            visible: Api.isLemmy
            iconName: "server"
            text: qsTr("Local")
            detail: Api.instance
            current: root.currentFeed === "local"
            onClicked: root.navigate(() => Nav.goToFeed("local"))
        }
        NavRow {
            iconName: "search"
            text: qsTr("Search")
            onClicked: root.navigate(() => Nav.openSearch("", ""))
        }
        NavRow {
            iconName: "bookmark"
            text: qsTr("Saved")
            detail: Library.savedCount > 0 ? String(Library.savedCount) : ""
            onClicked: root.navigate(() => Nav.openSaved())
        }
        NavRow {
            iconName: "history"
            text: qsTr("History")
            onClicked: root.navigate(() => Nav.openHistory())
        }
        Divider {}

        // Communities
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.topMargin: 6
            Layout.bottomMargin: 4
            Text {
                Layout.fillWidth: true
                text: qsTr("Communities")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
            }
            Text {
                visible: !Account.signedIn
                text: qsTr("Import")
                color: Theme.accent
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                rightPadding: 8
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: root.importRequested() }
            }
        }
        Text {
            visible: root.communities.length === 0
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.bottomMargin: 6
            text: Account.signedIn ? qsTr("Join communities to see them here.")
                                   : qsTr("Join communities to build your Home feed.")
            color: Theme.textFaint
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.Wrap
        }
        Repeater {
            model: root.communities
            delegate: NavRow {
                required property string modelData
                avatarName: modelData
                text: Api.community(modelData)
                compact: true
                current: root.currentFeed === modelData.toLowerCase()
                onClicked: root.navigate(() => Nav.goToFeed(modelData))
            }
        }
        Divider {}

        NavRow {
            iconName: "settings"
            text: qsTr("Settings")
            onClicked: root.navigate(() => Nav.openSettings())
        }
        NavRow {
            iconName: "info"
            text: qsTr("About")
            onClicked: root.navigate(() => Nav.openAbout())
        }
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 6
        Layout.bottomMargin: 6
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        Layout.preferredHeight: 1
        color: Theme.divider
    }

    component NavRow: Rectangle {
        id: nav
        property string iconName
        property string avatarName
        property string avatarUrl
        property string text
        property string detail
        property bool current: false
        property bool compact: false
        signal clicked()

        Layout.fillWidth: true
        implicitHeight: compact ? 36 : 42
        radius: 10
        color: current ? Theme.accentSoft : navHover.hovered ? Theme.hover : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 12
            Icon {
                visible: nav.iconName !== ""
                name: nav.iconName
                size: 19
                color: nav.current ? Theme.accent : Theme.textMuted
            }
            Avatar {
                visible: nav.avatarName !== ""
                name: nav.avatarName
                imageUrl: nav.avatarUrl
                size: nav.compact ? 20 : 24
                backgroundColor: nav.color
            }
            Text {
                Layout.fillWidth: true
                text: nav.text
                color: nav.current ? Theme.accent : Theme.text
                font.pixelSize: nav.compact ? Theme.fontMeta + 1 : Theme.fontBody - 1
                font.weight: nav.current ? Font.DemiBold : Font.Medium
                elide: Text.ElideRight
            }
            Text {
                visible: nav.detail !== ""
                text: nav.detail
                color: Theme.textFaint
                font.pixelSize: Theme.fontSmall
            }
        }
        HoverHandler { id: navHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: nav.clicked() }
    }
}
