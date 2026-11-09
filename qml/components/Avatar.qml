import QtQuick

// A round avatar: the community/user icon when there is one, otherwise a colourful letter.
// `backgroundColor` must match what the avatar sits on (used to round the image corners).
Item {
    id: root
    property string name
    property string imageUrl
    property int size: 32
    property color backgroundColor: Theme.surface

    width: size
    height: size
    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Format.nameColor(root.name)
        visible: !img.visible

        Text {
            anchors.centerIn: parent
            text: {
                const n = root.name.replace(/^(r|u)\//i, "")
                return n.length > 0 ? n.charAt(0).toUpperCase() : "?"
            }
            color: "#FFFFFF"
            font.pixelSize: Math.round(root.size * 0.48)
            font.weight: Font.Bold
        }
    }

    RoundedImage {
        id: img
        anchors.fill: parent
        visible: root.imageUrl !== "" && status === Image.Ready
        source: root.imageUrl
        radius: root.size / 2
        backgroundColor: root.backgroundColor
        placeholderColor: "transparent"
    }
}
