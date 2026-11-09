import QtQuick
import QtQuick.Layouts

// A community in a list: icon, name, members and description.
Item {
    id: row
    required property string name
    required property string title
    required property string description
    required property var subscribers
    required property string iconUrl
    required property bool nsfw
    required property var communityId
    required property var activeMonth   // -1 when unknown
    required property var posts         // -1 when unknown

    signal clicked()
    property alias action: actionSlot.data

    // The item spans the list; the visible box is a centred column.
    implicitHeight: box.implicitHeight

    Rectangle {
        id: box
        width: Math.min(row.width - 24, Theme.contentMaxWidth)
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
        implicitHeight: content.implicitHeight + 28
        radius: Theme.radiusCard
        color: hover.hovered ? Theme.hover : Theme.bg
        Rectangle {
            anchors.bottom: parent.bottom
            x: 12
            width: parent.width - 24
            height: 1
            color: Theme.divider
        }

        RowLayout {
            id: content
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12
            Avatar {
                Layout.alignment: Qt.AlignTop
                name: row.name
                imageUrl: row.iconUrl
                size: 44
                backgroundColor: box.color
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                RowLayout {
                    spacing: 6
                    Text {
                        text: Api.community(row.name)
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.Bold
                    }
                    Chip { visible: row.nsfw; text: "NSFW"; fontSize: 10; hpad: 6; fillColor: Theme.alpha(Theme.nsfw, 0.14); textColor: Theme.nsfw }
                }
                Text {
                    text: {
                        const parts = [qsTr("%1 members").arg(Format.compact(row.subscribers))]
                        if (row.activeMonth > 0)
                            parts.push(qsTr("%1 active a month").arg(Format.compact(row.activeMonth)))
                        if (row.posts > 0)
                            parts.push(qsTr("%1 posts").arg(Format.compact(row.posts)))
                        return parts.join("  ·  ")
                    }
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSmall
                }
                Text {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: row.description !== "" ? row.description : row.title
                    color: Theme.text
                    font.pixelSize: Theme.fontMeta
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
            Item {
                id: actionSlot
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: childrenRect.width
                implicitHeight: childrenRect.height
            }
        }

        HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
        TapHandler { id: tap; onTapped: row.clicked() }
    }
}
