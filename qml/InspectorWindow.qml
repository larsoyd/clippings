// facts about the picture and the chosen annotation, as Grab's and Preview's inspectors have them

import QtQuick
import QtQuick.Window
import "shell" as Shell
import "shell/ThemeValues.js" as ThemeValues
import "Look.js" as Look

Window {
    id: inspector

    property bool wanted: false
    visible: wanted && !app.ownWindowsHidden
    onClosing: wanted = false

    required property var document

    readonly property var desktop: settings
    readonly property var font: settings.theme.fonts.interface

    title: "Inspector"
    width: 320
    height: 180
    minimumWidth: 320
    maximumWidth: 320
    color: Look.background(settings)

    component Line: Row {
        property string name
        property string value
        spacing: 8
        Text {
            width: 110
            text: parent.name
            horizontalAlignment: Text.AlignRight
            font.family: inspector.font.family
            font.pixelSize: 12
            font.bold: true
            color: ThemeValues.color(inspector.desktop.theme.controls?.text_color ?? "#000000")
        }
        Text {
            text: parent.value
            font.family: inspector.font.family
            font.pixelSize: 12
            color: ThemeValues.color(inspector.desktop.theme.controls?.text_color ?? "#000000")
        }
    }

    Column {
        x: 16
        y: 16
        spacing: 6
        Line { name: "Image Size:"; value: inspector.document.imageSize.width + " × " + inspector.document.imageSize.height + " pixels" }
        Line { name: "Density:"; value: inspector.document.density + "×" }
        Line { name: "Annotations:"; value: inspector.document.annotationCount }
        Line { name: "File:"; value: inspector.document.path !== "" ? inspector.document.title : "Not saved" }
        Line { name: "Line Width:"; value: inspector.document.lineWidth + " pt" }
    }
}
