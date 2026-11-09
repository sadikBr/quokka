import QtQuick

// Faster, direct mouse-wheel and touchpad scrolling for a Flickable / ListView.
// Qt Quick's default turns each wheel step into a slow kinetic flick; this moves the content by a
// fixed distance per step (smoothly), and follows touchpad gestures pixel for pixel.
// Place it as a sibling of the view: WheelScroller { flickable: list }
MouseArea {
    id: root
    property Flickable flickable
    property real stepSize: 150          // pixels per wheel notch
    anchors.fill: flickable
    acceptedButtons: Qt.NoButton         // clicks and touches go through to the view

    function clamp(y) {
        const f = flickable
        const min = f.originY - f.topMargin
        const max = Math.max(min, f.originY + f.contentHeight - f.height + f.bottomMargin)
        return Math.max(min, Math.min(max, y))
    }

    onWheel: (wheel) => {
        if (!flickable || (wheel.modifiers & Qt.ControlModifier)) {
            wheel.accepted = false
            return
        }
        if (wheel.pixelDelta.y !== 0) {
            // Touchpad: follow the fingers directly.
            anim.stop()
            flickable.cancelFlick()
            flickable.contentY = clamp(flickable.contentY - wheel.pixelDelta.y)
        } else if (wheel.angleDelta.y !== 0) {
            const from = anim.running ? anim.to : flickable.contentY
            anim.stop()
            flickable.cancelFlick()
            anim.from = flickable.contentY
            anim.to = clamp(from - wheel.angleDelta.y / 120 * stepSize)
            anim.start()
        }
        wheel.accepted = true
    }

    NumberAnimation {
        id: anim
        target: root.flickable
        property: "contentY"
        duration: 160
        easing.type: Easing.OutCubic
    }
}
