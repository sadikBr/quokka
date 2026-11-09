import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Side drawer: navigation, your communities and settings.
Drawer {
    id: root
    width: Math.min(340, parent ? parent.width * 0.86 : 340)
    edge: Qt.LeftEdge

    // When true the drawer is shown permanently as a sidebar (wide desktop windows).
    property bool docked: false
    signal importRequested()

    function go(action) {
        action()
        if (!docked)
            close()
    }

    background: Rectangle {
        color: Theme.bg
        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.border
        }
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: DrawerContent {
        onNavigate: (action) => root.go(action)
        onImportRequested: root.importRequested()
    }
}
