// a problem that belongs to no document

import QtQuick
import QtQuick.Window
import "shell" as Shell
import "Look.js" as Look

Window {
    id: alert

    readonly property var desktop: settings

    property bool wanted: false
    visible: wanted && !app.ownWindowsHidden
    onClosing: wanted = false

    title: "Clippings"
    width: 417
    height: 136
    color: Look.background(settings)

    Connections {
        target: app
        function onAlert(message, detail) {
            alert.wanted = true
            sheet.open(message, detail, ["OK"])
        }
    }

    Shell.AlertSheet {
        id: sheet
        settings: alert.desktop
        iconSource: app.iconSource
        onChosen: alert.wanted = false
    }
}
