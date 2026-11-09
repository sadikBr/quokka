import QtQuick
import QtQuick.Controls

// A slim scroll bar. On touch screens it is only an indicator (scroll with the finger) and
// fades out when scrolling stops; with a mouse it can be dragged and grows a little on hover.
ScrollBar {
    id: bar
    readonly property bool touch: Qt.platform.os === "android" || Qt.platform.os === "ios"
    interactive: !touch
    policy: ScrollBar.AsNeeded
    padding: touch ? 2 : 3
    minimumSize: 0.04

    contentItem: Rectangle {
        implicitWidth: bar.touch ? 3 : (bar.hovered || bar.pressed ? 8 : 5)
        implicitHeight: implicitWidth
        radius: width / 2
        color: Theme.alpha(Theme.text, bar.pressed ? 0.5 : bar.hovered ? 0.38 : 0.28)
        opacity: bar.active && bar.size < 1 ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 300 } }
        Behavior on implicitWidth { NumberAnimation { duration: 120 } }
    }
    background: Item {}
}
