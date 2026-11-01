// the editor for one text annotation, placed and sized as the text will land in the picture

import QtQuick

Item {
    id: entry

    required property var canvas
    required property var document

    property real sourceX: 0
    property real sourceY: 0
    property int index: -1

    anchors.fill: parent
    visible: false

    function begin(x, y, annotation) {
        sourceX = x
        sourceY = y
        index = annotation
        input.text = document.text_of(annotation)
        visible = true
        input.forceActiveFocus()
        input.selectAll()
    }

    function finish() {
        if (!visible) {
            return
        }
        visible = false
        if (index >= 0) {
            document.set_text(index, input.text)
        } else {
            document.add_text(sourceX, sourceY, input.text)
        }
        document.tool = 0
        canvas.forceActiveFocus()
    }

    Rectangle {
        x: entry.canvas.view_x_of(entry.sourceX) - 2
        y: entry.canvas.view_y_of(entry.sourceY) - 2
        width: Math.max(40, input.implicitWidth + 6)
        height: input.implicitHeight + 4
        color: "#30ffffff"
        border.color: "#266ed8"

        TextEdit {
            id: input
            x: 2
            y: 2
            color: entry.document.strokeColor
            font.family: entry.document.fontFamily
            font.pixelSize: Math.max(4, entry.document.fontSize * entry.canvas.zoom)
            Accessible.role: Accessible.EditableText
            Accessible.name: "Text"
            Keys.onEscapePressed: entry.finish()
            onActiveFocusChanged: {
                if (!activeFocus) {
                    entry.finish()
                }
            }
        }
    }
}
