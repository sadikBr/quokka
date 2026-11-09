pragma Singleton
import QtQuick

// Design tokens: a calm, content-first look. Everything visual reads from here.
QtObject {
    id: theme

    // Qt.styleHints.colorScheme: 1 = light, 2 = dark (Qt 6.5+).
    readonly property bool systemDark: Qt.styleHints.colorScheme === 2
    readonly property bool dark: AppSettings.themeMode === AppSettings.Dark
                                 || (AppSettings.themeMode === AppSettings.System && systemDark)

    // Accent: one warm coral, a nod to the quokka. Used sparingly.
    readonly property color accent: "#F0506E"
    readonly property color accentSoft: dark ? "#3A1B24" : "#FDE8EC"
    readonly property color textOnAccent: "#FFFFFF"

    // Supporting colours (rails, avatars, small highlights)
    readonly property color berry: accent
    readonly property color caramel: "#F5A524"
    readonly property color mint: "#30A46C"
    readonly property color sky: "#3E8BFF"
    readonly property color grape: "#8E4EC6"

    // Surfaces: flat. Content sits on the background; dividers separate it.
    readonly property color bg: dark ? "#0E0E11" : "#FFFFFF"
    readonly property color surface: bg
    readonly property color surfaceAlt: dark ? "#1B1B20" : "#F3F3F5"   // chips, inputs, placeholders
    readonly property color sidebar: dark ? "#121216" : "#F8F8FA"
    readonly property color hover: dark ? "#16161B" : "#F7F7F9"
    readonly property color border: dark ? "#2A2A31" : "#E4E4E9"
    readonly property color divider: dark ? "#1E1E24" : "#EDEDF0"
    readonly property color shadow: "transparent"
    readonly property color text: dark ? "#EDEDF2" : "#141418"
    readonly property color textMuted: dark ? "#9696A2" : "#6B6B76"
    readonly property color textFaint: dark ? "#6C6C78" : "#9A9AA4"
    readonly property color scrim: dark ? "#B3000000" : "#66000000"
    readonly property color popup: dark ? "#18181D" : "#FFFFFF"

    // Semantic
    readonly property color upvote: "#FF6B3D"
    readonly property color nsfw: "#E5484D"
    readonly property color spoiler: "#7C66DC"
    readonly property color stickied: mint

    // Shape and spacing
    readonly property int radiusCard: 14
    readonly property int radiusMedia: 10
    readonly property int radiusChip: 8
    readonly property int gap: 12
    readonly property int pad: 16
    readonly property int contentMaxWidth: 720

    // Type (Inter, bundled)
    readonly property string fontFamily: "Inter"
    readonly property int fontTitle: 16
    readonly property int fontBody: 15
    readonly property int fontMeta: 13
    readonly property int fontSmall: 12
    readonly property int fontHuge: 22

    // Motion
    readonly property int quick: 120
    readonly property int smooth: 200

    // Theme.qml is served from the module root (see QT_RESOURCE_ALIAS in CMakeLists.txt).
    function icon(name) {
        return Qt.resolvedUrl("resources/icons/" + name + ".svg")
    }
    readonly property url logo: Qt.resolvedUrl("resources/logo.svg")

    function alpha(c, a) {
        const col = Qt.lighter(c, 1.0)
        return Qt.rgba(col.r, col.g, col.b, a)
    }

    // Flair colours are arbitrary; keep text readable on top of them.
    function readableOn(c) {
        const col = Qt.lighter(c, 1.0)
        const l = 0.299 * col.r + 0.587 * col.g + 0.114 * col.b
        return l > 0.6 ? "#141418" : "#FFFFFF"
    }
}
