import QtQuick
import QtQuick.Layouts

// A community in a list: icon, name, members and description.
Rectangle {
    id: row
    required property string name
    required property string title
    required property string description
    required property var subscribers
    required property string iconUrl
    required property bool nsfw

    signal clicked()

    implicitHeight: content.implicitHeight + 24
    radius: Theme.radiusMedia + 2
    color: hover.hovered ? Theme.surfaceAlt : Theme.surface
    border.width: 1
    border.color: Theme.border
    scale: tap.pressed ? 0.98 : 1
    Behavior on scale { NumberAnimation { duration: Theme.quick } }

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        Avatar {
            Layout.alignment: Qt.AlignTop
            name: row.name
            imageUrl: row.iconUrl
            size: 44
            backgroundColor: row.color
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
                Chip { visible: row.nsfw; text: "NSFW"; fontSize: 10; hpad: 6; fillColor: Theme.nsfw; textColor: "white"; border.width: 0 }
            }
            Text {
                text: qsTr("%1 members").arg(Format.compact(row.subscribers))
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
    property alias action: actionSlot.data

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { id: tap; onTapped: row.clicked() }
}
