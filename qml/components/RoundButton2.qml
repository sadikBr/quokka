import QtQuick
import QtQuick.Controls

// A round, bouncy icon button.
Item {
    id: root
    property string iconName
    property int size: 40
    property color tint: Theme.text
    property color fill: Theme.surfaceAlt
    property string tooltip
    signal clicked()

    implicitWidth: size
    implicitHeight: size
    Accessible.role: Accessible.Button
    Accessible.name: tooltip

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.fill
        scale: tap.pressed ? 0.88 : (hover.hovered ? 1.06 : 1.0)
        Behavior on scale { NumberAnimation { duration: Theme.quick; easing.type: Easing.OutBack } }

        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: Math.round(root.size * 0.5)
            color: root.tint
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    ToolTip.visible: hover.hovered && tooltip !== "" && Qt.platform.os !== "android"
    ToolTip.text: tooltip
    ToolTip.delay: 600
    TapHandler { id: tap; gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: root.clicked() }
}
