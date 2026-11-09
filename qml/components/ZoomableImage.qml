import QtQuick

// A pannable, zoomable image: pinch, Ctrl/plain mouse wheel, or double-tap to zoom.
Flickable {
    id: flick
    property url source
    property url fallbackSource   // tried when `source` fails (e.g. the server's own thumbnail)
    // Every copy to try, in order (see media::imageSources): a failed one moves on to the next.
    readonly property var sources: MediaResolver.imageSources(source.toString(), fallbackSource.toString())
    property int attempt: 0
    onSourcesChanged: attempt = 0
    readonly property url effectiveSource: attempt < sources.length ? sources[attempt] : ""
    readonly property bool broken: (status === Image.Error && attempt >= sources.length - 1)
                                   || (sources.length === 0 && (source.toString() !== "" || fallbackSource.toString() !== ""))
    // Shown (already cached from the feed) while the full-size picture loads.
    property url placeholderSource
    property bool animated: false
    property real zoom: 1
    readonly property real maxZoom: 6
    readonly property bool zoomed: zoom > 1.01
    readonly property int status: content.item ? content.item.status : Image.Null
    onStatusChanged: {
        if (status === Image.Error) {
            MediaResolver.reportImage(effectiveSource.toString(), false)
            if (attempt < sources.length - 1)
                Qt.callLater(() => attempt++)
        }
        if (status === Image.Ready)
            MediaResolver.reportImage(effectiveSource.toString(), true)
    }
    signal tapped()

    clip: true
    contentWidth: width * zoom
    contentHeight: height * zoom
    boundsBehavior: Flickable.StopAtBounds
    interactive: zoomed

    function zoomTo(z, cx, cy) {
        z = Math.max(1, Math.min(maxZoom, z))
        const fx = (contentX + cx) / contentWidth
        const fy = (contentY + cy) / contentHeight
        zoom = z
        contentX = Math.max(0, Math.min(contentWidth - width, fx * contentWidth - cx))
        contentY = Math.max(0, Math.min(contentHeight - height, fy * contentHeight - cy))
    }
    function reset() { zoom = 1; contentX = 0; contentY = 0 }

    Image {
        width: flick.contentWidth
        height: flick.contentHeight
        visible: flick.status !== Image.Ready && !flick.broken && status === Image.Ready
        source: flick.status === Image.Ready ? "" : flick.placeholderSource
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize.width: 640
    }
    Loader {
        id: content
        width: flick.contentWidth
        height: flick.contentHeight
        sourceComponent: flick.animated ? animatedImage : staticImage
    }
    Component {
        id: staticImage
        Image {
            source: flick.effectiveSource
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 2560
            smooth: true
            mipmap: true
        }
    }
    Component {
        id: animatedImage
        AnimatedImage {
            source: flick.effectiveSource
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            playing: true
        }
    }

    PinchArea {
        id: pinch
        width: Math.max(flick.contentWidth, flick.width)
        height: Math.max(flick.contentHeight, flick.height)
        property real startZoom: 1
        onPinchStarted: startZoom = flick.zoom
        onPinchUpdated: (p) => flick.zoomTo(startZoom * p.scale, p.center.x - flick.contentX, p.center.y - flick.contentY)
        onPinchFinished: flick.returnToBounds()
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: (event) => {
            const factor = event.angleDelta.y > 0 ? 1.18 : 1 / 1.18
            flick.zoomTo(flick.zoom * factor, point.position.x, point.position.y)
        }
    }
    TapHandler {
        onDoubleTapped: (ep) => flick.zoomTo(flick.zoomed ? 1 : 2.5, ep.position.x, ep.position.y)
        onSingleTapped: flick.tapped()
    }
}
