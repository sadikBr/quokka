import QtQuick

// Faster, direct mouse-wheel and touchpad scrolling for a Flickable / ListView.
// Qt Quick's default turns each wheel step into a slow kinetic flick; this moves the content by a
// fixed distance per wheel step, smoothly. Place it as a sibling of the view:
//   WheelScroller { flickable: list }
MouseArea {
    id: root
    property Flickable flickable
    // Pixels per wheel notch (120 units of angleDelta), scaled by the user's scroll speed.
    property real stepSize: 180 * AppSettings.scrollSpeed
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
        // Prefer wheel steps: on X11 a plain mouse wheel may also report a small pixelDelta,
        // which would make every notch crawl. Touchpads report steps too (in small fractions).
        let dy = 0
        if (wheel.angleDelta.y !== 0)
            dy = wheel.angleDelta.y / 120 * stepSize
        else if (wheel.pixelDelta.y !== 0)
            dy = wheel.pixelDelta.y * AppSettings.scrollSpeed
        if (dy === 0) {
            wheel.accepted = false
            return
        }
        const from = anim.running ? anim.to : flickable.contentY
        anim.stop()
        flickable.cancelFlick()
        anim.from = flickable.contentY
        anim.to = clamp(from - dy)
        anim.start()
        wheel.accepted = true
    }

    NumberAnimation {
        id: anim
        target: root.flickable
        property: "contentY"
        duration: 140
        easing.type: Easing.OutCubic
    }
}
