import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Full-screen viewer for a post's images, gallery items and videos.
Page {
    id: page
    property var post: ({})
    property int startIndex: 0
    property bool showCommentsButton: true
    readonly property bool active: StackView.status === StackView.Active
    property bool chrome: true

    readonly property var items: {
        const m = post.media || []
        if (m.length > 0)
            return m
        return post.previewUrl ? [{ url: post.previewUrl, width: post.previewWidth || 0,
                                    height: post.previewHeight || 0, caption: "", animated: false }] : []
    }
    readonly property var current: items.length > 0 ? items[Math.max(0, pager.currentIndex)] : null

    function close() { page.StackView.view.pop() }
    function isVideoUrl(u) { return /\.(m3u8|mp4)(\?|$)/i.test(u) || /HLSPlaylist|DASH_/i.test(u) }
    function toggleFullScreen() {
        const w = Nav.window
        if (w) w.visibility = w.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
    }

    background: Rectangle { color: "#0B0A0F" }

    Shortcut { sequences: ["Right", "L"]; enabled: page.active; onActivated: pager.incrementCurrentIndex() }
    Shortcut { sequences: ["Left", "H"]; enabled: page.active; onActivated: pager.decrementCurrentIndex() }
    Shortcut { sequences: ["F", "F11"]; enabled: page.active; onActivated: page.toggleFullScreen() }
    Shortcut { sequences: ["Q"]; enabled: page.active; onActivated: page.close() } // Esc is handled by Main

    Component.onDestruction: {
        const w = Nav.window
        if (w && w.visibility === Window.FullScreen)
            w.visibility = Window.Windowed
    }

    ListView {
        id: pager
        anchors.fill: parent
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 220
        boundsBehavior: Flickable.StopAtBounds
        model: page.items
        currentIndex: page.startIndex
        interactive: !(currentItem && currentItem.zoomed)
        cacheBuffer: width
        Component.onCompleted: positionViewAtIndex(page.startIndex, ListView.Beginning)

        delegate: Item {
            id: slot
            required property var modelData
            required property int index
            width: pager.width
            height: pager.height
            readonly property bool isCurrent: ListView.isCurrentItem
            readonly property bool isVideo: page.isVideoUrl(modelData.url)
            readonly property bool zoomed: loader.item && loader.item.zoomed !== undefined ? loader.item.zoomed : false

            Loader {
                id: loader
                anchors.fill: parent
                active: Math.abs(slot.index - pager.currentIndex) <= 1
                sourceComponent: slot.isVideo ? videoComp : imageComp
            }
            Component {
                id: imageComp
                ZoomableImage {
                    source: slot.modelData.url
                    fallbackSource: slot.modelData.fallbackUrl || ""
                    animated: /\.gif(\?|$)/i.test(slot.modelData.url)
                    onTapped: page.chrome = !page.chrome
                    Connections {
                        target: slot
                        function onIsCurrentChanged() { if (!slot.isCurrent) parent.reset() }
                    }
                    LoadingDots {
                        anchors.centerIn: parent
                        visible: parent.status === Image.Loading
                    }
                    Column {
                        anchors.centerIn: parent
                        visible: parent.broken
                        spacing: 8
                        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "image-off"; size: 40; color: "white" }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("This image couldn't be loaded. Its server may be down.")
                            color: "white"
                            font.pixelSize: Theme.fontBody
                        }
                    }
                }
            }
            Component {
                id: videoComp
                VideoPlayer {
                    source: slot.modelData.url
                    fallbackSource: slot.index === 0 && page.post.videoFallbackUrl ? page.post.videoFallbackUrl : ""
                    active: slot.isCurrent && page.active
                    showControls: page.chrome
                    onTapped: page.chrome = !page.chrome
                }
            }
        }
    }

    // ---- Top bar ----
    Rectangle {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        opacity: page.chrome ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.quick } }
        gradient: Gradient {
            GradientStop { position: 0; color: "#CC000000" }
            GradientStop { position: 1; color: "#00000000" }
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8
            RoundButton2 {
                iconName: "close"
                fill: "#33FFFFFF"
                tint: "white"
                tooltip: qsTr("Close")
                onClicked: page.close()
            }
            Text {
                Layout.fillWidth: true
                text: page.post.title || ""
                color: "white"
                font.pixelSize: Theme.fontBody
                font.weight: Font.Bold
                elide: Text.ElideRight
            }
            Chip {
                visible: page.items.length > 1
                text: (pager.currentIndex + 1) + " / " + page.items.length
                fillColor: "#33FFFFFF"
                textColor: "white"
                border.width: 0
            }
            RoundButton2 {
                visible: page.showCommentsButton && !!page.post.postId
                iconName: "comment"
                fill: "#33FFFFFF"
                tint: "white"
                tooltip: qsTr("Comments")
                onClicked: {
                    const p = page.post
                    page.close()
                    Nav.openPost(p)
                }
            }
            RoundButton2 {
                visible: Qt.platform.os !== "android"
                iconName: "fullscreen"
                fill: "#33FFFFFF"
                tint: "white"
                tooltip: qsTr("Full screen (F)")
                onClicked: page.toggleFullScreen()
            }
            RoundButton2 {
                iconName: "external"
                fill: "#33FFFFFF"
                tint: "white"
                tooltip: qsTr("Open in browser")
                onClicked: Qt.openUrlExternally(page.current ? page.current.url : page.post.url)
            }
        }
    }

    // ---- Caption ----
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: page.current && page.isVideoUrl(page.current.url) ? 64 : 0
        height: captionText.implicitHeight + 32
        visible: page.chrome && !!page.current && !!page.current.caption
        gradient: Gradient {
            GradientStop { position: 0; color: "#00000000" }
            GradientStop { position: 1; color: "#CC000000" }
        }
        Text {
            id: captionText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            text: page.current ? (page.current.caption || "") : ""
            color: "white"
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontBody
        }
    }

    // ---- Desktop arrows ----
    RoundButton2 {
        visible: page.chrome && page.items.length > 1 && pager.currentIndex > 0
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        size: 48
        iconName: "chevron-left"
        fill: "#55000000"
        tint: "white"
        tooltip: qsTr("Previous")
        onClicked: pager.decrementCurrentIndex()
    }
    RoundButton2 {
        visible: page.chrome && page.items.length > 1 && pager.currentIndex < page.items.length - 1
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        size: 48
        iconName: "chevron-right"
        fill: "#55000000"
        tint: "white"
        tooltip: qsTr("Next")
        onClicked: pager.incrementCurrentIndex()
    }

    // Dots
    Row {
        visible: page.items.length > 1 && page.items.length <= 20 && page.chrome
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16 + (page.current && page.current.caption ? captionText.implicitHeight + 24 : 0)
        spacing: 6
        Repeater {
            model: page.items.length
            Rectangle {
                required property int index
                width: index === pager.currentIndex ? 18 : 7
                height: 7
                radius: 3.5
                color: index === pager.currentIndex ? Theme.accent : "#88FFFFFF"
                Behavior on width { NumberAnimation { duration: Theme.quick } }
            }
        }
    }
}
