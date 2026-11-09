import QtQuick
import QtQuick.Layouts

// Compact header for secondary pages: back button, title, optional actions on the right.
Rectangle {
    id: root
    property string title
    property string subtitle
    property string avatarName
    property bool showBack: true
    default property alias actions: actionRow.data
    signal back()
    signal titleClicked()

    implicitHeight: 64
    color: Theme.bg

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        RoundButton2 {
            visible: root.showBack
            iconName: "chevron-down"
            rotation: 90
            fill: Theme.alpha(Theme.accent, 0.14)
            tint: Theme.accent
            tooltip: qsTr("Back")
            onClicked: root.back()
        }
        Avatar {
            visible: root.avatarName !== ""
            name: root.avatarName
            size: 34
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                Layout.fillWidth: true
                text: root.title
                color: Theme.text
                font.pixelSize: Theme.fontTitle + 1
                font.weight: Font.Black
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: text !== ""
                text: root.subtitle
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
            TapHandler { onTapped: root.titleClicked() }
        }
        Row {
            id: actionRow
            spacing: 6
        }
    }
}
