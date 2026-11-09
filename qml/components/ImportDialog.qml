import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Paste a list of communities to join them all at once (or copy yours out).
Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 460, 480)
    padding: 20
    property string result

    onOpened: { area.text = ""; result = ""; area.forceActiveFocus() }

    background: Rectangle {
        radius: Theme.radiusCard + 2
        color: Theme.popup
        border.width: 1
        border.color: Theme.border
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: ColumnLayout {
        spacing: 12
        Text {
            text: qsTr("Import communities")
            color: Theme.text
            font.pixelSize: Theme.fontHuge
            font.weight: Font.Bold
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Paste community names separated by spaces, commas or new lines, "
                       + "for example: r/pics, aww, linux")
            color: Theme.textMuted
            font.pixelSize: Theme.fontMeta
            wrapMode: Text.Wrap
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            radius: Theme.radiusMedia
            color: Theme.surfaceAlt
            border.width: area.activeFocus ? 2 : 1
            border.color: area.activeFocus ? Theme.accent : Theme.border
            ScrollView {
                anchors.fill: parent
                anchors.margins: 4
                TextArea {
                    id: area
                    wrapMode: TextEdit.Wrap
                    color: Theme.text
                    placeholderText: "r/pics\nr/aww\nr/linux"
                    placeholderTextColor: Theme.textMuted
                    background: null
                    selectByMouse: true
                }
            }
        }
        Text {
            visible: root.result !== ""
            Layout.fillWidth: true
            text: root.result
            color: Theme.mint
            font.pixelSize: Theme.fontMeta
            font.weight: Font.Bold
            wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Chip {
                text: qsTr("Show my list")
                iconName: "layout"
                clickable: Library.subscriptions.length > 0
                opacity: Library.subscriptions.length > 0 ? 1 : 0.5
                onClicked: {
                    area.text = Library.exportSubscriptions()
                    area.selectAll()
                    root.result = qsTr("Your list is selected; copy it with Ctrl+C.")
                }
            }
            Item { Layout.fillWidth: true }
            Chip {
                text: qsTr("Close")
                clickable: true
                onClicked: root.close()
            }
            Chip {
                text: qsTr("Import")
                iconName: "plus"
                selected: true
                clickable: true
                onClicked: {
                    const n = Library.importSubscriptions(area.text)
                    root.result = n === 0 ? qsTr("Nothing new to add.")
                                : n === 1 ? qsTr("Joined 1 community!") : qsTr("Joined %1 communities!").arg(n)
                }
            }
        }
    }
}
