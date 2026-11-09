import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Sort tabs (Hot / New / Top / Active / Controversial) with an underline, plus a time-range
// menu for Top and Controversial.
Item {
    id: root
    property string sort: "hot"
    property string timeRange: "day"
    signal sortPicked(string sort)
    signal timeRangePicked(string range)

    implicitHeight: 44

    readonly property var sorts: [
        { key: "hot", label: qsTr("Hot") },
        { key: "new", label: qsTr("New") },
        { key: "top", label: qsTr("Top") },
        { key: "active", label: qsTr("Active") },
        { key: "controversial", label: qsTr("Controversial") }
    ]
    readonly property var ranges: [
        { key: "hour", label: qsTr("Past hour") }, { key: "day", label: qsTr("Today") },
        { key: "week", label: qsTr("This week") }, { key: "month", label: qsTr("This month") },
        { key: "year", label: qsTr("This year") }, { key: "all", label: qsTr("All time") }
    ]
    readonly property bool timed: sort === "top" || sort === "controversial"
    function rangeLabel(key) {
        for (const r of ranges)
            if (r.key === key)
                return r.label
        return key
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: tabs.width
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick

            Row {
                id: tabs
                height: parent.height
                spacing: 2
                Repeater {
                    model: root.sorts
                    delegate: Item {
                        id: tab
                        required property var modelData
                        readonly property bool current: root.sort === modelData.key
                        width: label.implicitWidth + 24
                        height: tabs.height
                        Rectangle {
                            anchors.fill: parent
                            anchors.topMargin: 6
                            anchors.bottomMargin: 6
                            radius: 8
                            color: tabHover.hovered && !tab.current ? Theme.hover : "transparent"
                        }
                        Text {
                            id: label
                            anchors.centerIn: parent
                            text: tab.modelData.label
                            color: tab.current ? Theme.text : Theme.textMuted
                            font.pixelSize: Theme.fontMeta + 1
                            font.weight: tab.current ? Font.DemiBold : Font.Medium
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: tab.current ? label.implicitWidth + 4 : 0
                            height: 3
                            radius: 1.5
                            color: Theme.accent
                            Behavior on width { NumberAnimation { duration: Theme.smooth; easing.type: Easing.OutCubic } }
                        }
                        HoverHandler { id: tabHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: root.sortPicked(tab.modelData.key) }
                    }
                }
            }
        }

        // Time range for Top / Controversial
        Rectangle {
            visible: root.timed
            Layout.preferredHeight: 30
            Layout.preferredWidth: rangeRow.implicitWidth + 20
            Layout.leftMargin: 8
            radius: 15
            color: rangeHover.hovered ? Qt.darker(Theme.surfaceAlt, Theme.dark ? 0.8 : 1.04) : Theme.surfaceAlt
            Row {
                id: rangeRow
                anchors.centerIn: parent
                spacing: 4
                Icon { name: "clock"; size: 14; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: root.rangeLabel(root.timeRange)
                    color: Theme.text
                    font.pixelSize: Theme.fontSmall + 1
                    font.weight: Font.Medium
                    anchors.verticalCenter: parent.verticalCenter
                }
                Icon { name: "chevron-down"; size: 14; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
            }
            HoverHandler { id: rangeHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: rangeMenu.popup() }

            Menu {
                id: rangeMenu
                Repeater {
                    model: root.ranges
                    delegate: MenuItem {
                        required property var modelData
                        text: modelData.label
                        checkable: true
                        checked: root.timeRange === modelData.key
                        onTriggered: root.timeRangePicked(modelData.key)
                    }
                }
            }
        }
    }
}
