import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// About Quokka, privacy and third-party licenses.
Page {
    id: page
    function back() { page.StackView.view.pop() }
    background: Rectangle { color: Theme.bg }

    header: TopBar {
        title: qsTr("About Quokka")
        onBack: page.back()
    }

    readonly property var components: [
        { name: "Qt " + Format.qtVersion(), license: "LGPL-3.0",
          url: "https://www.qt.io/licensing/open-source-lgpl-obligations",
          note: qsTr("Application framework (Core, GUI, Quick, Network, Multimedia, SQL, SVG).") },
        { name: "FFmpeg", license: "LGPL-2.1+", url: "https://ffmpeg.org/legal.html",
          note: qsTr("Video decoding on desktop, through Qt Multimedia.") },
        { name: "OpenSSL 3", license: "Apache-2.0", url: "https://www.openssl.org/source/license.html",
          note: qsTr("HTTPS on Android (packaged by KDAB's android_openssl).") },
        { name: "SQLite", license: qsTr("Public domain"), url: "https://www.sqlite.org/copyright.html",
          note: qsTr("Stores your communities, saved posts and history.") }
    ]

    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + 48
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: col
            width: Math.min(parent.width - 32, Theme.contentMaxWidth)
            x: (parent.width - width) / 2
            y: 24
            spacing: 14

            Image {
                Layout.alignment: Qt.AlignHCenter
                source: Theme.logo
                sourceSize: Qt.size(112, 112)
                SequentialAnimation on rotation {
                    loops: Animation.Infinite
                    NumberAnimation { to: -5; duration: 1100; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 5; duration: 1100; easing.type: Easing.InOutSine }
                }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Quokka " + Qt.application.version
                color: Theme.text
                font.pixelSize: Theme.fontHuge
                font.weight: Font.Black
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("A happy little, read-only reader for Lemmy, on desktop and Android.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
            }

            Card {
                title: qsTr("Privacy")
                body: qsTr("Quokka has no account, no tracking and no server of its own. It reads the public "
                           + "API of the Lemmy server you choose (now: %1). Your communities, saved posts and "
                           + "history are stored only on this device.").arg(Api.serviceName)
            }
            Card {
                title: qsTr("Independent")
                body: qsTr("Quokka is an independent project, not affiliated with the Lemmy developers, any "
                           + "Lemmy server, or Reddit, Inc. Content belongs to its authors and is moderated by "
                           + "each community and server.")
            }

            Text {
                text: qsTr("Open-source components")
                color: Theme.accent
                font.pixelSize: Theme.fontSmall
                font.weight: Font.Black
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1.2
                Layout.topMargin: 8
            }
            Repeater {
                model: page.components
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: compRow.implicitHeight + 24
                    radius: Theme.radiusMedia
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.border
                    RowLayout {
                        id: compRow
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: modelData.name
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.Bold
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.note
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSmall
                                wrapMode: Text.Wrap
                            }
                        }
                        Chip {
                            text: modelData.license
                            clickable: true
                            onClicked: Qt.openUrlExternally(modelData.url)
                        }
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Qt is used under the LGPL: Quokka links to it dynamically, and you can replace "
                           + "the Qt libraries with your own build. Quokka's source code is on GitHub.")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
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

    component Card: Rectangle {
        property string title
        property string body
        Layout.fillWidth: true
        implicitHeight: cardCol.implicitHeight + 28
        radius: Theme.radiusCard
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        ColumnLayout {
            id: cardCol
            anchors.fill: parent
            anchors.margins: 14
            spacing: 4
            Text {
                text: parent.parent.title
                color: Theme.text
                font.pixelSize: Theme.fontBody
                font.weight: Font.Black
            }
            Text {
                Layout.fillWidth: true
                text: parent.parent.body
                color: Theme.textMuted
                font.pixelSize: Theme.fontMeta
                wrapMode: Text.Wrap
                lineHeight: 1.2
            }
        }
    }
}
