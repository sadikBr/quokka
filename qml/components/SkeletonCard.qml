import QtQuick

// Placeholder card shown while the first page loads.
Rectangle {
    id: root
    property bool withMedia: true
    implicitHeight: withMedia ? 330 : 170
    radius: Theme.radiusCard
    color: Theme.surface
    border.width: 1
    border.color: Theme.border

    SequentialAnimation on opacity {
        loops: Animation.Infinite
        NumberAnimation { to: 0.55; duration: 700; easing.type: Easing.InOutSine }
        NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
    }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: 12
        Row {
            spacing: 10
            Rectangle { width: 34; height: 34; radius: 17; color: Theme.surfaceAlt }
            Column {
                spacing: 6
                anchors.verticalCenter: parent.verticalCenter
                Rectangle { width: 110; height: 10; radius: 5; color: Theme.surfaceAlt }
                Rectangle { width: 70; height: 8; radius: 4; color: Theme.surfaceAlt }
            }
        }
        Rectangle { width: parent.width * 0.9; height: 14; radius: 7; color: Theme.surfaceAlt }
        Rectangle { width: parent.width * 0.6; height: 14; radius: 7; color: Theme.surfaceAlt }
        Rectangle {
            visible: root.withMedia
            width: parent.width
            height: 160
            radius: Theme.radiusMedia
            color: Theme.surfaceAlt
        }
        Row {
            spacing: 8
            Rectangle { width: 64; height: 26; radius: 13; color: Theme.surfaceAlt }
            Rectangle { width: 54; height: 26; radius: 13; color: Theme.surfaceAlt }
        }
    }
}
