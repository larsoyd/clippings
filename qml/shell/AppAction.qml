// one command, shared by its menu item, toolbar button and shortcut

import QtQml

QtObject {
    property string text: ""
    property string shortcut: ""
    property bool enabled: true
    property bool checkable: false
    property bool checked: false
    // a toolbar picture from the art folder, and the words where it's missing
    property string glyph: ""
    // a desktop icon by name, shown beside the small words
    property string icon: ""
    property string shortLabel: text

    signal triggered()

    function trigger() {
        if (enabled) {
            triggered()
        }
    }
}
