import QtQuick

// Renders post/comment HTML (cleaned Reddit HTML or rendered Lemmy Markdown). Links route through Nav;
// spoilers stay hidden until tapped.
Text {
    id: root
    property string html
    property bool spoilersRevealed: false
    readonly property bool hasSpoilers: html.indexOf("QK_SPOILER") >= 0

    readonly property string css: "<style>"
        + "a { color: " + Theme.accent + "; text-decoration: none; font-weight: 600; }"
        + "code { background-color: " + Theme.surfaceAlt + "; font-family: monospace; }"
        + "pre { background-color: " + Theme.surfaceAlt + "; font-family: monospace; }"
        + "blockquote { color: " + Theme.textMuted + "; }"
        + "th { background-color: " + Theme.surfaceAlt + "; }"
        + "p { margin-top: 0px; margin-bottom: 8px; }"
        + "</style>"
    readonly property string spoilerCss: spoilersRevealed
        ? "background-color: " + Theme.alpha(Theme.spoiler, 0.25)
        : "background-color: " + Theme.spoiler + "; color: " + Theme.spoiler

    text: css + html.replace(/QK_SPOILER/g, spoilerCss)
    textFormat: Text.RichText
    wrapMode: Text.Wrap
    color: Theme.text
    linkColor: Theme.accent
    font.pixelSize: Theme.fontBody
    lineHeight: 1.12
    onLinkActivated: (link) => Nav.openLink(link)

    HoverHandler {
        cursorShape: root.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
    TapHandler {
        enabled: root.hasSpoilers && !root.spoilersRevealed
        onTapped: root.spoilersRevealed = true
    }
}
