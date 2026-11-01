// the theme's menu box inside a window, with a click catcher that dismisses it

import QtQuick

Item {
    id: popup

    required property var settings
    required property var options
    property int current: -1
    property point openAt: Qt.point(0, 0)
    property bool opened: false
    property Item returnFocusTo: null

    signal chosenIndex(int index)

    anchors.fill: parent
    visible: opened
    z: 1000

    function close() {
        opened = false
        if (returnFocusTo !== null) {
            returnFocusTo.forceActiveFocus()
        }
    }

    Keys.onEscapePressed: close()
    Keys.onUpPressed: box.innermost().moveMark(-1)
    Keys.onDownPressed: box.innermost().moveMark(1)
    Keys.onReturnPressed: box.innermost().chooseMarked()
    Keys.onSpacePressed: box.innermost().chooseMarked()

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: popup.close()
    }

    MenuBox {
        id: box
        settings: popup.settings
        items: popup.options.map((label, index) => ({ id: index + 1, label: label, enabled: true, separator: false,
                                                      checkable: true, checked: index === popup.current, shortcut: [], children: [] }))
        x: Math.max(0, Math.min(popup.openAt.x, popup.width - width))
        y: Math.max(0, Math.min(popup.openAt.y, popup.height - height))
        onChosen: id => {
            popup.close()
            popup.chosenIndex(id - 1)
        }
    }
}
