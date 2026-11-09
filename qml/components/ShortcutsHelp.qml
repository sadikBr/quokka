import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Keyboard shortcuts cheat sheet (press ? or F1).
Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 520, 560)
    height: Math.min(parent ? parent.height - 48 : 600, content.implicitHeight + 48)
    padding: 24

    background: Rectangle {
        radius: Theme.radiusCard + 4
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    readonly property var groups: [
        { title: qsTr("Everywhere"), keys: [
            ["Esc / Backspace / Alt+←", qsTr("Back")], ["Home", qsTr("Back to the main feed")],
            ["Ctrl+F", qsTr("Search")], ["G / Ctrl+L", qsTr("Go to a community")],
            ["R / F5", qsTr("Refresh")], ["U", qsTr("Scroll to top")], ["? / F1", qsTr("This help")] ] },
        { title: qsTr("Feeds"), keys: [
            ["J / K", qsTr("Next / previous post")], ["Enter / O", qsTr("Open post")],
            ["M", qsTr("Open media")], ["S", qsTr("Save / unsave")] ] },
        { title: qsTr("Post"), keys: [
            ["J / K", qsTr("Next / previous top-level comment")], ["C", qsTr("Collapse / expand it")],
            ["M", qsTr("Open media")], ["S", qsTr("Save / unsave")] ] },
        { title: qsTr("Media viewer"), keys: [
            ["← / →", qsTr("Previous / next")], ["F / F11", qsTr("Full screen")],
            [qsTr("Wheel / double-click"), qsTr("Zoom")], ["Q / Esc", qsTr("Close")] ] }
    ]

    contentItem: Flickable {
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ColumnLayout {
            id: content
            width: parent.width
            spacing: 6
            Text {
                text: qsTr("Keyboard shortcuts")
                color: Theme.text
                font.pixelSize: Theme.fontHuge
                font.weight: Font.Black
                Layout.bottomMargin: 6
            }
            Repeater {
                model: root.groups
                delegate: ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        text: modelData.title
                        color: Theme.accent
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.Black
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.2
                        Layout.topMargin: 8
                    }
                    Repeater {
                        model: modelData.keys
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Rectangle {
                                Layout.preferredWidth: 200
                                implicitHeight: keyText.implicitHeight + 8
                                radius: 8
                                color: Theme.surfaceAlt
                                border.width: 1
                                border.color: Theme.border
                                Text {
                                    id: keyText
                                    anchors.centerIn: parent
                                    text: modelData[0]
                                    color: Theme.text
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.Bold
                                    font.family: "monospace"
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.leftMargin: 8
                                text: modelData[1]
                                color: Theme.text
                                font.pixelSize: Theme.fontMeta
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }
    }
}
