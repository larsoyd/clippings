// asks for a place to save where no file chooser can, drawn like AlertSheet from the theme's [dialog]

import QtQuick
import QtQuick.Window
import "ThemeValues.js" as ThemeValues

Item {
    id: sheet

    required property var settings
    property string message: ""
    property string detail: ""
    property int topInset: 0
    property bool opened: false

    signal accepted(string path)
    signal cancelled()

    readonly property var looks: settings.theme.dialog ?? ({})
    readonly property var fonts: settings.theme.fonts

    anchors.fill: parent
    visible: opened || slide.running
    z: 900

    function open(path, words, more) {
        message = words
        detail = more
        input.text = path
        opened = true
        input.forceActiveFocus()
        input.select(input.text.lastIndexOf("/") + 1, input.text.lastIndexOf("."))
    }
    function save() {
        if (input.text.trim() === "") {
            return
        }
        opened = false
        accepted(input.text.trim())
    }
    function cancel() {
        opened = false
        cancelled()
    }

    MouseArea {
        anchors.fill: parent
    }

    Item {
        anchors.fill: parent
        anchors.topMargin: sheet.topInset
        clip: true

        Rectangle {
            id: panel
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(sheet.looks.width ?? 417, sheet.width)
            height: words.y + words.height + 12 + field.height + 8 + 32 + 14
            y: sheet.opened ? 0 : -height
            color: ThemeValues.color(sheet.looks.background ?? "#ededed")

            Behavior on y {
                NumberAnimation {
                    id: slide
                    duration: 180
                    easing.type: Easing.OutQuad
                }
            }

            Column {
                id: words
                x: 20
                y: 14
                width: panel.width - 2 * x
                spacing: sheet.looks.text_gap ?? 6
                Text {
                    width: parent.width
                    text: sheet.message
                    wrapMode: Text.Wrap
                    color: ThemeValues.color(sheet.looks.text_color ?? "#000000")
                    font.family: sheet.fonts.dialog_title?.family ?? sheet.fonts.interface.family
                    font.pixelSize: sheet.fonts.dialog_title?.size ?? 13
                    font.bold: sheet.fonts.dialog_title?.bold ?? true
                }
                Text {
                    width: parent.width
                    text: sheet.detail
                    wrapMode: Text.Wrap
                    color: ThemeValues.color(sheet.looks.text_color ?? "#000000")
                    font.family: sheet.fonts.dialog_text?.family ?? sheet.fonts.interface.family
                    font.pixelSize: sheet.fonts.dialog_text?.size ?? 11
                }
            }
            Rectangle {
                id: field
                x: words.x
                y: words.y + words.height + 12
                width: words.width
                height: 24
                color: ThemeValues.color(sheet.settings.theme.controls?.base_color ?? "#ffffff")
                border.width: 1
                border.color: input.activeFocus ? Qt.rgba(0.32, 0.6, 0.86, 1) : "#8a8a8a"
                TextInput {
                    id: input
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    verticalAlignment: TextInput.AlignVCenter
                    clip: true
                    selectByMouse: true
                    color: ThemeValues.color(sheet.settings.theme.controls?.text_color ?? "#000000")
                    font.family: sheet.fonts.interface.family
                    font.pixelSize: sheet.fonts.interface.size
                    Keys.onEscapePressed: sheet.cancel()
                    Keys.onReturnPressed: sheet.save()
                    Keys.onEnterPressed: sheet.save()
                }
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: sheet.looks.button_inset?.[0] ?? 14
                anchors.bottom: parent.bottom
                anchors.bottomMargin: sheet.looks.button_inset?.[1] ?? 12
                PushButton {
                    width: wantedWidth
                    looks: sheet.looks
                    fonts: sheet.fonts
                    themeDirectory: sheet.settings.theme_directory
                    text: "Cancel"
                    onClicked: sheet.cancel()
                }
                PushButton {
                    width: wantedWidth
                    looks: sheet.looks
                    fonts: sheet.fonts
                    themeDirectory: sheet.settings.theme_directory
                    text: "Save"
                    isDefault: true
                    onClicked: sheet.save()
                }
            }
        }
    }
}
