import QtQuick

// A colourful letter bubble for a subreddit or user.
Rectangle {
    id: root
    property string name
    property int size: 32

    width: size
    height: size
    radius: size / 2
    color: Format.nameColor(name)

    Text {
        anchors.centerIn: parent
        text: root.name.length > 0 ? root.name.charAt(0).toUpperCase() : "?"
        color: "#FFFFFF"
        font.pixelSize: Math.round(root.size * 0.48)
        font.weight: Font.Black
    }
}
