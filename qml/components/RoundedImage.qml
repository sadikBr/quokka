import QtQuick

// An image with rounded corners. Instead of a mask effect, a thick border in the
// surrounding colour covers the corners: cheap, and works with every Qt version and GPU backend.
Item {
    id: root
    property alias source: img.source
    property alias fillMode: img.fillMode
    property alias status: img.status
    property int radius: Theme.radiusMedia
    property color backgroundColor: Theme.surface   // colour of whatever is behind the image
    property color placeholderColor: Theme.surfaceAlt
    // height / width of the loaded picture, 0 until it has loaded
    readonly property real naturalRatio: img.status === Image.Ready && img.implicitWidth > 0
                                         ? img.implicitHeight / img.implicitWidth : 0

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.placeholderColor
        clip: true

        // Shimmer while loading
        Rectangle {
            visible: img.status === Image.Loading
            width: parent.width * 0.35
            height: parent.height
            opacity: 0.35
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.5; color: Theme.dark ? "#4A4260" : "#FFFFFF" }
                GradientStop { position: 1; color: "transparent" }
            }
            NumberAnimation on x {
                running: img.status === Image.Loading
                loops: Animation.Infinite
                from: -root.width * 0.35
                to: root.width
                duration: 1100
            }
        }
    }

    Image {
        id: img
        anchors.fill: parent
        asynchronous: true
        cache: true
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: Math.max(1, Math.ceil(root.width * Screen.devicePixelRatio))
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.smooth } }
    }

    // Corner cover: inner edge of this border is a rounded rect matching `root`. The image
    // corner sits radius*(sqrt(2)-1) outside the arc, so a border of ~0.42*radius is enough.
    Rectangle {
        readonly property int bw: Math.ceil(root.radius * 0.45) + 1
        x: -bw
        y: -bw
        width: root.width + 2 * bw
        height: root.height + 2 * bw
        radius: root.radius + bw
        color: "transparent"
        border.width: bw
        border.color: root.backgroundColor
    }
}
