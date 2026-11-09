import QtQuick

// A pill-shaped tab switcher.
Rectangle {
    id: root
    property var tabs: []
    property int currentIndex: 0
    signal picked(int index)

    implicitHeight: 42
    radius: height / 2
    color: Theme.surfaceAlt
    border.width: 1
    border.color: Theme.border

    Rectangle {
        id: thumb
        y: 4
        height: parent.height - 8
        width: (root.width - 8) / Math.max(1, root.tabs.length)
        x: 4 + root.currentIndex * width
        radius: height / 2
        color: Theme.accent
        Behavior on x { NumberAnimation { duration: Theme.smooth; easing.type: Easing.OutBack } }
    }

    Row {
        anchors.fill: parent
        anchors.margins: 4
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
                    color: parent.index === root.currentIndex ? Theme.textOnAccent : Theme.text
                    font.pixelSize: Theme.fontMeta
                    font.weight: Font.Bold
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
