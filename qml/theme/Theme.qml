pragma Singleton
import QtQuick

// Design tokens. Everything visual reads from here so light/dark and future tweaks stay in one place.
QtObject {
    id: theme

    // Qt.styleHints.colorScheme: 1 = light, 2 = dark (Qt 6.5+).
    readonly property bool systemDark: Qt.styleHints.colorScheme === 2
    readonly property bool dark: AppSettings.themeMode === AppSettings.Dark
                                 || (AppSettings.themeMode === AppSettings.System && systemDark)

    // Brand
    readonly property color berry: "#FF5E7E"
    readonly property color caramel: "#FFB547"
    readonly property color mint: "#2ECDA7"
    readonly property color sky: "#6C8CFF"
    readonly property color grape: "#A66CFF"
    readonly property color accent: berry
    readonly property color textOnAccent: "#FFFFFF"

    // Surfaces
    readonly property color bg: dark ? "#15121C" : "#FFF6EC"
    readonly property color surface: dark ? "#211D2B" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#2C2739" : "#FCEBDC"
    readonly property color border: dark ? "#3A3349" : "#F0D9C4"
    readonly property color shadow: dark ? "#0B0910" : "#F2DCC8"
    readonly property color text: dark ? "#F7F1FF" : "#2A1E1A"
    readonly property color textMuted: dark ? "#A99FBC" : "#8A7266"
    readonly property color scrim: dark ? "#CC0B0910" : "#992A1E1A"

    // Semantic
    readonly property color upvote: "#FF7A45"
    readonly property color nsfw: "#FF3B5C"
    readonly property color spoiler: "#7C6CFF"
    readonly property color stickied: mint

    // Shape and spacing
    readonly property int radiusCard: 22
    readonly property int radiusMedia: 16
    readonly property int radiusChip: 14
    readonly property int gap: 12
    readonly property int pad: 16
    readonly property int contentMaxWidth: 720

    // Type scale
    readonly property int fontTitle: 17
    readonly property int fontBody: 15
    readonly property int fontMeta: 13
    readonly property int fontSmall: 12
    readonly property int fontHuge: 26

    // Motion
    readonly property int quick: 140
    readonly property int smooth: 260

    // Theme.qml is served from the module root (see QT_RESOURCE_ALIAS in CMakeLists.txt).
    function icon(name) {
        return Qt.resolvedUrl("resources/icons/" + name + ".svg")
    }
    readonly property url logo: Qt.resolvedUrl("resources/logo.svg")

    function alpha(c, a) {
        const col = Qt.lighter(c, 1.0)
        return Qt.rgba(col.r, col.g, col.b, a)
    }

    // Reddit flair colours are arbitrary; keep text readable on top of them.
    function readableOn(c) {
        const col = Qt.lighter(c, 1.0)
        const l = 0.299 * col.r + 0.587 * col.g + 0.114 * col.b
        return l > 0.6 ? "#2A1E1A" : "#FFFFFF"
    }
}
