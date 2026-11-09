import QtQuick
import QtQuick.Layouts

// Sort chips (hot/new/top/...) plus a time-range row for top/controversial.
ColumnLayout {
    id: root
    property string sort: "hot"
    property string timeRange: "day"
    signal sortPicked(string sort)
    signal timeRangePicked(string range)

    spacing: 8

    readonly property var sorts: [
        { key: "hot", label: qsTr("Hot"), icon: "hot", color: Theme.berry },
        { key: "new", label: qsTr("New"), icon: "new", color: Theme.caramel },
        { key: "top", label: qsTr("Top"), icon: "top", color: Theme.mint },
        { key: "active", label: qsTr("Active"), icon: "rising", color: Theme.sky },
        { key: "controversial", label: qsTr("Spicy"), icon: "controversial", color: Theme.grape }
    ]
    readonly property var ranges: [
        { key: "hour", label: qsTr("Hour") }, { key: "day", label: qsTr("Today") },
        { key: "week", label: qsTr("Week") }, { key: "month", label: qsTr("Month") },
        { key: "year", label: qsTr("Year") }, { key: "all", label: qsTr("All time") }
    ]

    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        orientation: ListView.Horizontal
        spacing: 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.sorts
        delegate: Chip {
            required property var modelData
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            text: modelData.label
            iconName: modelData.icon
            fontSize: Theme.fontMeta
            hpad: 14
            clickable: true
            selected: root.sort === modelData.key
            selectedColor: modelData.color
            onClicked: root.sortPicked(modelData.key)
        }
    }

    ListView {
        id: rangeRow
        readonly property bool shown: root.sort === "top" || root.sort === "controversial"
        Layout.fillWidth: true
        Layout.preferredHeight: shown ? 30 : 0
        opacity: shown ? 1 : 0
        visible: Layout.preferredHeight > 0
        Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.smooth; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: Theme.smooth } }
        orientation: ListView.Horizontal
        spacing: 6
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.ranges
        delegate: Chip {
            required property var modelData
            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            text: modelData.label
            iconName: root.timeRange === modelData.key ? "clock" : ""
            clickable: true
            selected: root.timeRange === modelData.key
            selectedColor: Theme.text
            textColor: selected ? Theme.bg : Theme.textMuted
            onClicked: root.timeRangePicked(modelData.key)
        }
    }
}
