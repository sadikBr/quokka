import QtQuick

// Three bouncing dots in the brand colours.
Row {
    id: root
    property bool running: true
    spacing: 8

    Repeater {
        model: [Theme.berry, Theme.caramel, Theme.mint]
        delegate: Rectangle {
            id: dot
            required property color modelData
            required property int index
            width: 12
            height: 12
            radius: 6
            color: modelData
            transform: Translate { id: bounce }
            SequentialAnimation {
                running: root.running && root.visible
                loops: Animation.Infinite
                PauseAnimation { duration: dot.index * 120 }
                NumberAnimation { target: bounce; property: "y"; to: -10; duration: 260; easing.type: Easing.OutQuad }
                NumberAnimation { target: bounce; property: "y"; to: 0; duration: 260; easing.type: Easing.InQuad }
                PauseAnimation { duration: (2 - dot.index) * 120 + 200 }
            }
        }
    }
}
