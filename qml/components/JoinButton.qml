import QtQuick

// Join / leave a community: on the server when signed in, otherwise in the local Home list.
Chip {
    id: root
    property string name
    property var communityId: 0
    readonly property bool joined: Account.signedIn
        ? Account.follows.length >= 0 && Account.isFollowing(name)
        : Library.subscriptions.length >= 0 && Library.isSubscribed(name)

    text: joined ? qsTr("Joined") : qsTr("Join")
    iconName: joined ? "check" : "plus"
    clickable: true
    fontSize: Theme.fontMeta
    hpad: 12
    selected: !joined
    onClicked: Account.signedIn ? Account.toggleFollow(name, communityId) : Library.toggleSubscription(name)
}
