import QtQuick

// A single-line text input without decoration: the placeholder simply disappears when you
// type (Qt's Material TextField floats it up as a label, which clashes with our boxes).
FocusScope {
    id: root
    property alias text: input.text
    property string placeholderText
    property color placeholderTextColor: Theme.textFaint
    property alias color: input.color
    property alias font: input.font
    property alias echoMode: input.echoMode
    property alias inputMethodHints: input.inputMethodHints
    property alias horizontalAlignment: input.horizontalAlignment
    signal accepted()
    signal textEdited()
    signal editingFinished()

    function selectAll() { input.selectAll() }

    implicitHeight: Math.max(input.implicitHeight, 24)
    implicitWidth: 120

    TextInput {
        id: input
        anchors.fill: parent
        focus: true
        verticalAlignment: TextInput.AlignVCenter
        clip: true
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBody
        selectByMouse: true
        selectionColor: Theme.alpha(Theme.accent, 0.35)
        selectedTextColor: Theme.text
        onAccepted: root.accepted()
        onTextEdited: root.textEdited()
        onEditingFinished: root.editingFinished()
    }
    Text {
        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: input.horizontalAlignment
        visible: input.text.length === 0 && input.preeditText.length === 0
        text: root.placeholderText
        color: root.placeholderTextColor
        font: input.font
        elide: Text.ElideRight
    }
    HoverHandler { cursorShape: Qt.IBeamCursor }
}
