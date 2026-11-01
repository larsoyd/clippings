// Grab's timer window: what is about to happen and a way out

import QtQuick
import "shell" as Shell
import "shell/ThemeValues.js" as ThemeValues

Rectangle {
    id: countdown

    readonly property var theme: settings.theme

    color: ThemeValues.color(theme.dialog?.background ?? "#ededed")
    radius: 6
    border.color: "#40000000"
    focus: true
    Keys.onEscapePressed: session.cancel()

    Image {
        x: 12
        y: 12
        width: 64
        height: 64
        source: app.iconSource
        sourceSize: Qt.size(128, 128)
    }
    Text {
        x: 94
        y: 12
        width: 278
        text: "Clippings will capture in " + session.remaining + (session.remaining === 1 ? " second." : " seconds.") + " This window will not be included."
        wrapMode: Text.Wrap
        font.family: countdown.theme.fonts.interface.family
        font.pixelSize: countdown.theme.fonts.interface.size
    }
    Shell.PushButton {
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        width: Math.max(82, wantedWidth)
        looks: countdown.theme.dialog ?? ({})
        fonts: countdown.theme.fonts
        themeDirectory: settings.theme_directory
        text: "Cancel"
        onClicked: session.cancel()
    }
}
