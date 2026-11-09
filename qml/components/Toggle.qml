import QtQuick

// An on/off switch in Quokka's style.
Item {
    id: root
    property bool checked: false
    signal toggled(bool on)

    implicitWidth: 44
    implicitHeight: 26
    opacity: enabled ? 1 : 0.5
    Accessible.role: Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: checked

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: root.checked ? Theme.accent : (Theme.dark ? "#3A3A43" : "#D6D6DD")
        Behavior on color { ColorAnimation { duration: Theme.quick } }
        Rectangle {
            width: parent.height - 6
            height: width
            radius: width / 2
            y: 3
            x: root.checked ? parent.width - width - 3 : 3
            color: "white"
            Behavior on x { NumberAnimation { duration: Theme.smooth; easing.type: Easing.OutCubic } }
        }
    }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.toggled(!root.checked) }
}
