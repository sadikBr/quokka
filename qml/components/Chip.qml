import QtQuick

// A small pill: a label, a filter or a button.
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
    color: selected ? selectedColor
         : clickable && hover.hovered ? Qt.darker(fillColor, Theme.dark ? 0.8 : 1.04) : fillColor
    border.width: 0
    border.color: Theme.border
    opacity: tap.pressed ? 0.75 : 1
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
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    HoverHandler { id: hover; enabled: root.clickable; cursorShape: Qt.PointingHandCursor }
    TapHandler { id: tap; gesturePolicy: TapHandler.ReleaseWithinBounds; enabled: root.clickable; onTapped: root.clicked() }
}
