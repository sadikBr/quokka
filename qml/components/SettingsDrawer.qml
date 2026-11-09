import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: root
    width: Math.min(340, parent ? parent.width * 0.86 : 340)
    edge: Qt.LeftEdge

    background: Rectangle {
        color: Theme.bg
        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.border
        }
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + 40
        clip: true

        ColumnLayout {
            id: col
            x: 20
            y: 24
            width: parent.width - 40
            spacing: 18

            // Brand header
            RowLayout {
                spacing: 14
                Image {
                    source: Theme.logo
                    sourceSize: Qt.size(64, 64)
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 64
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

            SectionLabel { text: qsTr("Theme") }
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

            SectionLabel { text: qsTr("Content") }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: nsfwRow.implicitHeight + 24
                radius: Theme.radiusMedia
                color: Theme.surface
                border.width: 1
                border.color: Theme.border
                RowLayout {
                    id: nsfwRow
                    anchors.fill: parent
                    anchors.margins: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: qsTr("Show NSFW posts")
                            color: Theme.text
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Hidden unless you turn this on.")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSmall
                            wrapMode: Text.Wrap
                        }
                    }
                    Switch {
                        checked: AppSettings.showNsfw
                        onToggled: AppSettings.showNsfw = checked
                    }
                }
            }

            SectionLabel { text: qsTr("About") }
            Text {
                Layout.fillWidth: true
                text: qsTr("Quokka is an open-source, read-only client that uses Reddit's public JSON. "
                           + "It is not affiliated with or endorsed by Reddit, Inc.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontMeta
                wrapMode: Text.Wrap
                lineHeight: 1.2
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
    }
}
