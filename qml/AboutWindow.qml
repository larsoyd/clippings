// the standard about panel

import QtQuick
import QtQuick.Window
import "shell/ThemeValues.js" as ThemeValues
import "Look.js" as Look

Window {
    id: about

    property bool wanted: false
    visible: wanted && !app.ownWindowsHidden
    onClosing: wanted = false

    title: "About Clippings"
    width: 284
    height: 220
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    color: Look.background(settings)

    Column {
        anchors.centerIn: parent
        spacing: 8
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 128
            height: 128
            source: app.iconSource
            sourceSize: Qt.size(256, 256)
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Clippings"
            font.family: settings.theme.fonts.interface.family
            font.pixelSize: 14
            font.bold: true
            color: ThemeValues.color(settings.theme.controls?.text_color ?? "#000000")
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Version " + app.version
            font.family: settings.theme.fonts.interface.family
            font.pixelSize: 11
            color: ThemeValues.color(settings.theme.controls?.text_color ?? "#000000")
        }
    }
}
