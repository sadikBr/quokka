import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// All settings, grouped: account, server, appearance, content, storage, about.
Page {
    id: page
    function back() { page.StackView.view.pop() }

    background: Rectangle { color: Theme.bg }

    header: TopBar {
        title: qsTr("Settings")
        onBack: page.back()
    }

    Flickable {
        id: scroller
        anchors.fill: parent
        contentHeight: col.implicitHeight + 48
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: col
            width: Math.min(parent.width - 32, 640)
            x: (parent.width - width) / 2
            y: 16
            spacing: 8

            // ---- Account ----
            SectionTitle { text: qsTr("Account"); visible: Account.supported }
            Group {
                visible: Account.supported
                ActionRow {
                    visible: !Account.signedIn
                    iconName: "user"
                    text: qsTr("Sign in to %1").arg(Api.instance)
                    detail: qsTr("Use your subscriptions and account settings.")
                    onClicked: Nav.openSignIn()
                }
                ActionRow {
                    visible: Account.signedIn
                    iconName: "user"
                    text: Account.name !== "" ? Api.user(Account.name) : qsTr("Signed in")
                    detail: qsTr("Signed in on %1").arg(Api.instance)
                    onClicked: Nav.openUser(Account.name)
                }
                SwitchRow {
                    visible: Account.signedIn
                    text: qsTr("Show NSFW on my account")
                    detail: qsTr("Saved on %1. Needed for NSFW results in search.").arg(Api.instance)
                    checked: Account.showNsfw
                    enabled: !Account.busy
                    onToggled: (on) => Account.setShowNsfw(on)
                }
                ActionRow {
                    visible: Account.signedIn
                    text: qsTr("Sign out")
                    textColor: Theme.nsfw
                    chevron: false
                    onClicked: Account.signOut()
                }
            }
            Text {
                visible: Account.error !== "" && !Account.busy
                Layout.fillWidth: true
                text: Account.error
                color: Theme.nsfw
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.Wrap
            }

            // ---- Server ----
            SectionTitle { text: qsTr("Server"); visible: Api.isLemmy }
            Group {
                visible: Api.isLemmy
                Item {
                    Layout.fillWidth: true
                    implicitHeight: serverCol.implicitHeight + 28
                    ColumnLayout {
                        id: serverCol
                        x: 16
                        y: 14
                        width: parent.width - 32
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Lemmy is made of many servers that share content; you can read everything "
                                       + "from any of them. Your account belongs to the server you signed up on.")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall + 1
                            wrapMode: Text.Wrap
                            lineHeight: 1.2
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 6
                            Repeater {
                                model: ["lemmy.world", "lemmy.ml", "programming.dev", "sh.itjust.works", "lemmy.zip", "discuss.tchncs.de"]
                                delegate: Chip {
                                    required property string modelData
                                    text: modelData
                                    clickable: true
                                    fontSize: Theme.fontMeta
                                    hpad: 12
                                    selected: Api.instance === modelData
                                    onClicked: Api.setInstance(modelData)
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                radius: 10
                                color: Theme.surfaceAlt
                                border.width: serverField.activeFocus ? 1 : 0
                                border.color: Theme.accent
                                PlainField {
                                    id: serverField
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    placeholderText: qsTr("Another server, e.g. lemmy.ca")
                                    placeholderTextColor: Theme.textFaint
                                    color: Theme.text
                                    font.pixelSize: Theme.fontMeta + 1
                                    inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                                    onAccepted: if (Api.setInstance(text)) text = ""
                                }
                            }
                            Chip {
                                text: qsTr("Use")
                                selected: true
                                clickable: serverField.text.trim() !== ""
                                fontSize: Theme.fontMeta
                                hpad: 16
                                implicitHeight: 38
                                onClicked: if (Api.setInstance(serverField.text)) serverField.text = ""
                            }
                        }
                    }
                }
            }

            // ---- Appearance ----
            SectionTitle { text: qsTr("Appearance") }
            Group {
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 64
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Theme")
                            color: Theme.text
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.Medium
                        }
                        Segmented {
                            options: [
                                { value: AppSettings.System, label: qsTr("Auto") },
                                { value: AppSettings.Light, label: qsTr("Light") },
                                { value: AppSettings.Dark, label: qsTr("Dark") }
                            ]
                            value: AppSettings.themeMode
                            onPicked: (v) => AppSettings.themeMode = v
                        }
                    }
                }
                SwitchRow {
                    text: qsTr("Compact posts")
                    detail: qsTr("Small thumbnails instead of large pictures.")
                    checked: AppSettings.compactCards
                    onToggled: (on) => AppSettings.compactCards = on
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 64
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Scroll speed")
                            color: Theme.text
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.Medium
                        }
                        Segmented {
                            options: [
                                { value: 1.0, label: qsTr("Normal") },
                                { value: 1.6, label: qsTr("Fast") },
                                { value: 2.4, label: qsTr("Faster") }
                            ]
                            value: AppSettings.scrollSpeed
                            onPicked: (v) => AppSettings.scrollSpeed = v
                        }
                    }
                }
                SwitchRow {
                    text: qsTr("Dim posts you've read")
                    checked: AppSettings.dimVisited
                    onToggled: (on) => AppSettings.dimVisited = on
                }
            }

            // ---- Content ----
            SectionTitle { text: qsTr("Content") }
            Group {
                SwitchRow {
                    text: qsTr("Show NSFW posts")
                    detail: qsTr("Hidden unless you turn this on.")
                    checked: AppSettings.showNsfw
                    onToggled: (on) => AppSettings.showNsfw = on
                }
                SwitchRow {
                    visible: AppSettings.showNsfw
                    text: qsTr("Cover NSFW media")
                    detail: qsTr("Pictures and videos stay covered until you tap them. Turn off to show them right away.")
                    checked: AppSettings.coverNsfw
                    onToggled: (on) => AppSettings.coverNsfw = on
                }
                SwitchRow {
                    text: qsTr("Start videos muted")
                    checked: AppSettings.videoMuted
                    onToggled: (on) => AppSettings.videoMuted = on
                }
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 64
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Default sort")
                            color: Theme.text
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.Medium
                        }
                        Segmented {
                            options: [
                                { value: "hot", label: qsTr("Hot") }, { value: "new", label: qsTr("New") },
                                { value: "top", label: qsTr("Top") }, { value: "active", label: qsTr("Active") }
                            ]
                            value: AppSettings.defaultSort
                            onPicked: (v) => AppSettings.defaultSort = v
                        }
                    }
                }
            }

            // ---- Storage ----
            SectionTitle { text: qsTr("Storage") }
            Group {
                ActionRow {
                    id: clearCache
                    property bool done: false
                    iconName: done ? "check" : "trash"
                    text: done ? qsTr("Image cache cleared") : qsTr("Clear image cache")
                    chevron: false
                    onClicked: { Library.clearImageCache(); done = true }
                }
                ActionRow {
                    iconName: "history"
                    text: qsTr("Clear reading history")
                    detail: Library.historyCount === 1 ? qsTr("1 post") : qsTr("%1 posts").arg(Library.historyCount)
                    chevron: false
                    enabled: Library.historyCount > 0
                    onClicked: Library.clearHistory()
                }
            }

            // ---- About ----
            SectionTitle { text: qsTr("About") }
            Group {
                ActionRow {
                    iconName: "info"
                    text: qsTr("About & licenses")
                    onClicked: Nav.openAbout()
                }
                ActionRow {
                    visible: Qt.platform.os !== "android"
                    iconName: "keyboard"
                    text: qsTr("Keyboard shortcuts")
                    onClicked: Nav.shortcutsRequested()
                }
                ActionRow {
                    iconName: "github"
                    text: qsTr("Source code")
                    detail: "GitHub"
                    onClicked: Qt.openUrlExternally("https://github.com/sadikBr/Reddit-Client")
                }
            }
        }
    }
    WheelScroller {
        flickable: scroller
    }

    component SectionTitle: Text {
        Layout.topMargin: 14
        Layout.leftMargin: 4
        color: Theme.textMuted
        font.pixelSize: Theme.fontSmall + 1
        font.weight: Font.DemiBold
    }

    // A rounded group of rows with dividers between them.
    component Group: Rectangle {
        default property alias rows: groupCol.data
        Layout.fillWidth: true
        implicitHeight: groupCol.implicitHeight
        radius: Theme.radiusCard
        color: Theme.bg
        border.width: 1
        border.color: Theme.border
        clip: true
        ColumnLayout {
            id: groupCol
            width: parent.width
            spacing: 0
        }
    }

    component RowBase: Rectangle {
        Layout.fillWidth: true
        color: "transparent"
        // Divider above every row but the first visible one.
        Rectangle {
            visible: parent.y > 0
            width: parent.width - 16
            x: 16
            height: 1
            color: Theme.divider
        }
    }

    component SwitchRow: RowBase {
        id: sw
        property string text
        property string detail
        property bool checked
        signal toggled(bool on)
        implicitHeight: swRow.implicitHeight + 24
        opacity: enabled ? 1 : 0.5
        RowLayout {
            id: swRow
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 8
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: sw.text
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.Medium
                    wrapMode: Text.Wrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: sw.detail !== ""
                    text: sw.detail
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    wrapMode: Text.Wrap
                }
            }
            Toggle {
                checked: sw.checked
                enabled: sw.enabled
                onToggled: (on) => sw.toggled(on)
            }
        }
    }

    component ActionRow: RowBase {
        id: action
        property string iconName
        property string text
        property string detail
        property color textColor: Theme.text
        property bool chevron: true
        signal clicked()
        implicitHeight: Math.max(52, actionRow.implicitHeight + 24)
        opacity: enabled ? 1 : 0.5
        color: actionHover.hovered && enabled ? Theme.hover : "transparent"
        RowLayout {
            id: actionRow
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 12
            Icon {
                visible: action.iconName !== ""
                name: action.iconName
                size: 19
                color: Theme.textMuted
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: action.text
                    color: action.textColor
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: action.detail !== ""
                    text: action.detail
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall + 1
                    wrapMode: Text.Wrap
                }
            }
            Icon {
                visible: action.chevron
                name: "chevron-right"
                size: 16
                color: Theme.textFaint
            }
        }
        HoverHandler { id: actionHover; enabled: action.enabled; cursorShape: Qt.PointingHandCursor }
        TapHandler { enabled: action.enabled; onTapped: action.clicked() }
    }

    // Small segmented control.
    component Segmented: Rectangle {
        id: seg
        property var options: []
        property var value
        signal picked(var value)
        implicitHeight: 32
        implicitWidth: segRow.implicitWidth + 6
        radius: 9
        color: Theme.surfaceAlt
        Row {
            id: segRow
            anchors.centerIn: parent
            spacing: 2
            Repeater {
                model: seg.options
                delegate: Rectangle {
                    required property var modelData
                    readonly property bool current: seg.value === modelData.value
                    width: segLabel.implicitWidth + 20
                    height: 26
                    radius: 7
                    color: current ? Theme.bg : "transparent"
                    border.width: current ? 1 : 0
                    border.color: Theme.border
                    Text {
                        id: segLabel
                        anchors.centerIn: parent
                        text: parent.modelData.label
                        color: parent.current ? Theme.text : Theme.textMuted
                        font.pixelSize: Theme.fontSmall + 1
                        font.weight: parent.current ? Font.DemiBold : Font.Medium
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    TapHandler { onTapped: seg.picked(parent.modelData.value) }
                }
            }
        }
    }
}
