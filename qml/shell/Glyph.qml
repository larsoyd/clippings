// a template picture with the etched highlight of a tool bar; words where the picture is missing

import QtQuick
import QtQuick.Window
import "ThemeValues.js" as ThemeValues

Item {
    id: glyph

    required property string folder
    required property string name
    property string fallback: ""
    property int side: 18
    property real inkOpacity: 0.78
    property var labelFont: ({ family: "sans-serif", size: 11 })

    implicitWidth: picture.status === Image.Ready ? side : words.implicitWidth
    implicitHeight: side

    Image {
        anchors.horizontalCenter: picture.horizontalCenter
        y: picture.y + 1
        visible: picture.status === Image.Ready
        source: picture.status === Image.Ready ? "file://" + glyph.folder + "/" + glyph.name + "-white.png" : ""
        sourceSize: Qt.size(glyph.side * 2, glyph.side * 2)
        width: glyph.side
        height: glyph.side
        fillMode: Image.PreserveAspectFit
        opacity: glyph.inkOpacity * 0.7
    }
    Image {
        id: picture
        anchors.centerIn: parent
        source: glyph.name !== "" ? "file://" + glyph.folder + "/" + glyph.name + ".png" : ""
        sourceSize: Qt.size(glyph.side * 2, glyph.side * 2)
        width: glyph.side
        height: glyph.side
        fillMode: Image.PreserveAspectFit
        opacity: glyph.inkOpacity
    }
    Text {
        id: words
        anchors.centerIn: parent
        visible: picture.status !== Image.Ready
        text: glyph.fallback
        font.family: glyph.labelFont.family
        font.pixelSize: glyph.labelFont.size
        opacity: glyph.inkOpacity
    }
}
