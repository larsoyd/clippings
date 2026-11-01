// a window tool bar's ground, lighter while the window isn't active

import QtQuick
import QtQuick.Window
import "ThemeValues.js" as ThemeValues

Item {
    id: bar

    required property var theme
    required property string themeDirectory
    // set where a bar sits under another and carries on from it
    property bool lower: false

    readonly property var look: theme.controls?.tool_bar ?? ({})
    readonly property var active: Window.active ? (look.fill ?? ["#d1d1d1", "#b0b0b0"]) : (look.fill_inactive ?? ["#eeeeee", "#e0e0e0"])
    readonly property var line: Window.active ? (look.bottom_line ?? "#696969") : (look.bottom_line_inactive ?? "#a6a6a6")

    Fill {
        anchors.fill: parent
        themeDirectory: bar.themeDirectory
        fill: ({ gradient: bar.active })
    }
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: ThemeValues.color(bar.line)
    }
}
