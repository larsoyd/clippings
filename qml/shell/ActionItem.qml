// a menu item that shows an action

import Qt.labs.platform as Platform

Platform.MenuItem {
    required property var action

    text: action.text
    shortcut: action.shortcut
    enabled: action.enabled
    checkable: action.checkable
    checked: action.checked
    onTriggered: action.trigger()
}
