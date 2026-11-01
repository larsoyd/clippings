// the keys of a capture

import QtQuick
import QtQuick.Window
import "shell/ThemeValues.js" as ThemeValues
import "Look.js" as Look

Window {
    id: help

    property bool wanted: false
    visible: wanted && !app.ownWindowsHidden
    onClosing: wanted = false

    title: "Clippings Help"
    width: 440
    height: 280
    color: Look.background(settings)

    Text {
        x: 20
        y: 18
        width: parent.width - 40
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        lineHeight: 1.25
        color: ThemeValues.color(settings.theme.controls?.text_color ?? "#000000")
        font.family: settings.theme.fonts.interface.family
        font.pixelSize: settings.theme.fonts.interface.size
        text: "Print Screen starts a capture and freezes the picture.\n\n"
            + "Drag to choose a part, then drag its edges to adjust it.\n"
            + "Return captures, Escape cancels.\n"
            + "Arrows move the selection, Alt and arrows resize it, Shift makes the step larger.\n"
            + "Space switches between a selection and a window, Tab goes through every mode.\n\n"
            + "Your picture opens in a new window. Mark it up, then use Copy or Save As in its tool bar."
    }
}
