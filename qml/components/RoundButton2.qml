import QtQuick
import QtQuick.Controls

// A quiet round icon button: transparent until hovered or pressed.
Item {
    id: root
    property string iconName
    property int size: 36
    property color tint: Theme.textMuted
    property color fill: "transparent"
    property string tooltip
    signal clicked()

    implicitWidth: size
    implicitHeight: size
    Accessible.role: Accessible.Button
    Accessible.name: tooltip

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: tap.pressed ? Theme.alpha(Theme.text, 0.12)
             : hover.hovered ? Theme.alpha(Theme.text, 0.07) : root.fill
        Behavior on color { ColorAnimation { duration: Theme.quick } }

        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: Math.round(root.size * 0.52)
            color: root.tint
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { id: tap; gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: root.clicked() }
    ToolTip.visible: hover.hovered && tooltip !== "" && Qt.platform.os !== "android"
    ToolTip.text: tooltip
    ToolTip.delay: 600
}
