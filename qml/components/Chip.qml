import QtQuick

// A pill-shaped label, optionally clickable and selectable.
Rectangle {
    id: root
    property string text
    property string iconName
    property bool selected: false
    property bool clickable: false
    property color fillColor: Theme.surfaceAlt
    property color selectedColor: Theme.accent
    property color textColor: selected ? Theme.textOnAccent : Theme.text
    property int fontSize: Theme.fontSmall
    property int hpad: 10
    signal clicked()

    implicitHeight: fontSize + 14
    implicitWidth: row.implicitWidth + hpad * 2
    radius: height / 2
    color: selected ? selectedColor : fillColor
    border.width: selected ? 0 : 1
    border.color: Theme.border
    scale: tap.pressed ? 0.93 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.quick; easing.type: Easing.OutBack } }
    Behavior on color { ColorAnimation { duration: Theme.quick } }

    Accessible.role: clickable ? Accessible.Button : Accessible.StaticText
    Accessible.name: text

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 5
        Icon {
            visible: root.iconName !== ""
            name: root.iconName
            size: root.fontSize + 2
            color: root.textColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.text
            color: root.textColor
            font.pixelSize: root.fontSize
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    HoverHandler { enabled: root.clickable; cursorShape: Qt.PointingHandCursor }
    TapHandler { id: tap; gesturePolicy: TapHandler.ReleaseWithinBounds; enabled: root.clickable; onTapped: root.clicked() }
}
