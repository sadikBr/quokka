import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Go to a community" sheet: type a name or pick a suggestion.
Popup {
    id: root
    signal picked(string feed)

    readonly property var suggestions: [
        "popular", "all", "pics", "aww", "AskReddit", "worldnews", "todayilearned",
        "technology", "science", "gaming", "movies", "mildlyinteresting", "programming",
        "linux", "android", "space", "food", "books"
    ]

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 420, 480)
    padding: 20
    onOpened: { field.text = ""; field.forceActiveFocus() }

    function submit(name) {
        const n = name.trim().replace(/^\/?r\//i, "")
        if (n.length === 0)
            return
        root.picked(n)
        root.close()
    }

    background: Rectangle {
        radius: Theme.radiusCard + 4
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    enter: Transition {
        NumberAnimation { property: "scale"; from: 0.85; to: 1; duration: Theme.smooth; easing.type: Easing.OutBack }
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.quick }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Theme.quick }
    }

    contentItem: ColumnLayout {
        spacing: 14

        Text {
            text: qsTr("Where to?")
            color: Theme.text
            font.pixelSize: Theme.fontHuge
            font.weight: Font.Black
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 25
            color: Theme.surfaceAlt
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.accent : Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 6
                spacing: 4
                Text {
                    text: "r/"
                    color: Theme.accent
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.Black
                }
                TextField {
                    id: field
                    Layout.fillWidth: true
                    placeholderText: qsTr("subreddit, or a+b+c")
                    placeholderTextColor: Theme.textMuted
                    color: Theme.text
                    font.pixelSize: Theme.fontTitle
                    background: null
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly
                    onAccepted: root.submit(text)
                }
                RoundButton2 {
                    iconName: "search"
                    size: 38
                    fill: Theme.accent
                    tint: Theme.textOnAccent
                    tooltip: qsTr("Go")
                    onClicked: root.submit(field.text)
                }
            }
        }

        Text {
            text: qsTr("Popular picks")
            color: Theme.textMuted
            font.pixelSize: Theme.fontMeta
            font.weight: Font.Bold
        }

        Flow {
            Layout.fillWidth: true
            spacing: 8
            Repeater {
                model: root.suggestions
                delegate: Chip {
                    required property string modelData
                    text: "r/" + modelData
                    fontSize: Theme.fontMeta
                    clickable: true
                    hpad: 12
                    fillColor: Theme.alpha(Format.nameColor(modelData), 0.16)
                    textColor: Theme.text
                    border.width: 0
                    onClicked: root.submit(modelData)
                }
            }
        }
    }
}
