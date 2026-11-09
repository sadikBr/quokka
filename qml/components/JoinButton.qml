import QtQuick

// Join / leave a community (stored locally; feeds your Home).
Chip {
    id: root
    property string name
    readonly property bool joined: Library.subscriptions.length >= 0 && Library.isSubscribed(name)

    text: joined ? qsTr("Joined") : qsTr("Join")
    iconName: joined ? "check" : "plus"
    clickable: true
    fontSize: Theme.fontMeta
    hpad: 12
    selected: !joined
    onClicked: Library.toggleSubscription(name)
}
