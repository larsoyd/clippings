import QtQuick
import QtQuick.Window
import "shell/ThemeValues.js" as ThemeValues

Window {
    id: window

    readonly property var controls: settings.theme.controls ?? ({})
    // the theme's own [clippings] table, when it has one
    readonly property var looks: settings.theme["clippings"] ?? ({})
    readonly property var words: (settings.config["clippings"] ?? ({})).words ?? ({})

    visible: true
    title: words.title ?? "Clippings"
    width: looks.width ?? 480
    height: looks.height ?? 320
    color: ThemeValues.color(looks.background ?? controls.window_color ?? "#ededed")

    Text {
        anchors.centerIn: parent
        text: window.title
        color: ThemeValues.color(window.controls.text_color ?? "#000000")
        font.family: settings.theme.fonts.interface.family
        font.pixelSize: settings.theme.fonts.interface.size
    }
}
