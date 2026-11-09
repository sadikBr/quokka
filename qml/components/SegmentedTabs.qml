import QtQuick

// A segmented switcher (e.g. Posts / Comments).
Rectangle {
    id: root
    property var tabs: []
    property int currentIndex: 0
    signal picked(int index)

    implicitHeight: 38
    radius: 10
    color: Theme.surfaceAlt

    Rectangle {
        id: thumb
        y: 3
        height: parent.height - 6
        width: (root.width - 6) / Math.max(1, root.tabs.length)
        x: 3 + root.currentIndex * width
        radius: 8
        color: Theme.bg
        border.width: 1
        border.color: Theme.border
        Behavior on x { NumberAnimation { duration: Theme.smooth; easing.type: Easing.OutCubic } }
    }

    Row {
        anchors.fill: parent
        anchors.margins: 3
        Repeater {
            model: root.tabs
            delegate: Item {
                required property var modelData
                required property int index
                width: thumb.width
                height: parent.height
                Text {
                    anchors.centerIn: parent
                    text: parent.modelData
                    color: parent.index === root.currentIndex ? Theme.text : Theme.textMuted
                    font.pixelSize: Theme.fontMeta
                    font.weight: parent.index === root.currentIndex ? Font.DemiBold : Font.Medium
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    onTapped: {
                        root.currentIndex = parent.index
                        root.picked(parent.index)
                    }
                }
            }
        }
    }
}
