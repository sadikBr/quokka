import QtQuick

// An image with rounded corners. Instead of a mask effect, a thick border in the
// surrounding colour covers the corners: cheap, and works with every Qt version and GPU backend.
Item {
    id: root
    property url source
    property url fallbackSource      // tried when `source` fails to load
    // Every copy to try, in order (see media::imageSources): a failed one moves on to the next.
    readonly property var sources: MediaResolver.imageSources(source.toString(), fallbackSource.toString())
    property int attempt: 0
    property bool showBroken: true   // show an "image unavailable" mark when nothing loads
    // Width to decode the picture at. Defaults to our width; callers whose width follows the
    // picture's shape pass a fixed width to avoid a feedback loop.
    property real decodeWidth: width
    property alias fillMode: img.fillMode
    property alias status: img.status
    property int radius: Theme.radiusMedia
    property color backgroundColor: Theme.surface   // colour of whatever is behind the image
    property color placeholderColor: Theme.surfaceAlt
    // height / width of the loaded picture, 0 until it has loaded. Set once on load (not a
    // binding): the image's implicit size follows its width, which may itself follow this ratio.
    property real naturalRatio: 0
    readonly property bool broken: img.status === Image.Error && attempt >= sources.length - 1
    onSourcesChanged: { attempt = 0; naturalRatio = 0 }

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
        source: root.attempt < root.sources.length ? root.sources[root.attempt] : ""
        onStatusChanged: {
            if (status === Image.Error && root.attempt < root.sources.length - 1)
                Qt.callLater(() => root.attempt++)
            if (status === Image.Ready && implicitWidth > 0)
                root.naturalRatio = implicitHeight / implicitWidth
        }
        asynchronous: true
        cache: true
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: Math.max(1, Math.ceil(root.decodeWidth * Screen.devicePixelRatio))
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.smooth } }
    }

    Column {
        anchors.centerIn: parent
        visible: root.broken && root.showBroken
        spacing: 4
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "image-off"
            size: Math.max(16, Math.min(32, root.width / 6))
            color: Theme.textMuted
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.width > 140
            text: qsTr("Image unavailable")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
        }
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
