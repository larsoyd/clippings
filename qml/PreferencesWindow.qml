// what stays between captures

import QtQuick
import QtQuick.Window
import Qt.labs.platform as Platform
import "shell" as Shell
import "shell/ThemeValues.js" as ThemeValues
import "Look.js" as Look

Window {
    id: preferences

    readonly property var desktop: settings

    title: "Clippings Preferences"
    width: 500
    height: 180
    color: Look.background(settings)

    Platform.FolderDialog {
        id: folderDialog
        title: "Choose a Folder"
        onAccepted: app.saveFolder = app.file_of(folder)
    }

    Column {
        x: 24
        y: 20
        spacing: 12
        Shell.CheckBox {
            settings: preferences.desktop
            text: "Play the capture sound"
            checked: app.playSound
            onToggled: checked => app.playSound = checked
        }
        Shell.CheckBox {
            settings: preferences.desktop
            text: "Show the pointer in captures"
            checked: session.pointer
            onToggled: checked => session.pointer = checked
        }
        Row {
            spacing: 12
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Save pictures in:  " + app.saveFolder
                width: 340
                elide: Text.ElideMiddle
                color: ThemeValues.color(preferences.desktop.theme.controls?.text_color ?? "#000000")
                font.family: preferences.desktop.theme.fonts.interface.family
                font.pixelSize: preferences.desktop.theme.fonts.interface.size
            }
            Shell.PushButton {
                width: wantedWidth
                looks: preferences.desktop.theme.dialog ?? ({})
                fonts: preferences.desktop.theme.fonts
                themeDirectory: preferences.desktop.theme_directory
                text: "Choose…"
                onClicked: folderDialog.open()
            }
        }
    }
}
