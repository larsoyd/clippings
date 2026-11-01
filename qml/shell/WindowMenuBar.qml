// the global menu of one window. the bar must be set on the window again once it is mapped,
// qt only announces its address to the compositor for a window that has a surface

import QtQuick
import Qt.labs.platform as Platform

Platform.MenuBar {
    id: bar

    required property var target

    property Timer rebind: Timer {
        interval: 0
        onTriggered: {
            bar.window = null
            bar.window = bar.target
        }
    }
    property Connections mapped: Connections {
        target: bar.target
        function onVisibleChanged() {
            if (bar.target.visible) {
                bar.rebind.restart()
            }
        }
    }

    Component.onCompleted: {
        if (target.visible) {
            rebind.restart()
        }
    }
}
