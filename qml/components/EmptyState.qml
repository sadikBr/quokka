import QtQuick
import QtQuick.Layouts

// Friendly full-page message with the mascot, e.g. for errors or empty feeds.
ColumnLayout {
    id: root
    property string title
    property string message
    property string actionText
    signal action()

    spacing: 14

    Image {
        id: logo
        Layout.alignment: Qt.AlignHCenter
        source: Theme.logo
        sourceSize: Qt.size(84, 84)
        Layout.preferredWidth: 84
        Layout.preferredHeight: 84
        SequentialAnimation on rotation {
            loops: Animation.Infinite
            NumberAnimation { to: -4; duration: 1200; easing.type: Easing.InOutSine }
            NumberAnimation { to: 4; duration: 1200; easing.type: Easing.InOutSine }
        }
    }
    Text {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: root.title
        color: Theme.text
        font.pixelSize: 20
        font.weight: Font.Bold
        wrapMode: Text.Wrap
    }
    Text {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: root.message
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
        wrapMode: Text.Wrap
        visible: text !== ""
    }
    Chip {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 6
        visible: root.actionText !== ""
        text: root.actionText
        iconName: "refresh"
        selected: true
        clickable: true
        fontSize: Theme.fontBody
        hpad: 18
        onClicked: root.action()
    }
}
