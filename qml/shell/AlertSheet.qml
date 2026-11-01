// a sheet hanging from under a window's tool bar, measured from the theme's [dialog]

import QtQuick
import QtQuick.Window
import "ThemeValues.js" as ThemeValues

Item {
    id: sheet

    required property var settings
    property string iconSource: ""
    property string message: ""
    property string detail: ""
    // left to right, the last is the default, return presses it
    property var buttons: []
    property int topInset: 0
    // what escape presses
    property int cancelIndex: 0
    property bool opened: false

    signal chosen(int index)

    readonly property var looks: settings.theme.dialog ?? ({})
    readonly property var fonts: settings.theme.fonts

    anchors.fill: parent
    visible: opened || slide.running
    z: 900

    function open(words, more, labels) {
        message = words
        detail = more
        buttons = labels
        opened = true
        forceActiveFocus()
    }
    function choose(index) {
        opened = false
        chosen(index)
    }

    Keys.onEscapePressed: choose(cancelIndex)
    Keys.onReturnPressed: choose(buttons.length - 1)
    Keys.onEnterPressed: choose(buttons.length - 1)

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
            height: Math.max(words.y + words.height, 76) + 8 + 32 + 14
            y: sheet.opened ? 0 : -height
            color: ThemeValues.color(sheet.looks.background ?? "#ededed")
            Accessible.role: Accessible.AlertMessage
            Accessible.name: sheet.message

            Behavior on y {
                NumberAnimation {
                    id: slide
                    duration: 180
                    easing.type: Easing.OutQuad
                }
            }

            Image {
                x: 12
                y: 12
                width: 64
                height: 64
                source: sheet.iconSource
            }
            Column {
                id: words
                x: sheet.looks.text_left ?? 93
                y: 14
                width: panel.width - x - 14
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
                    visible: sheet.detail !== ""
                    text: sheet.detail
                    wrapMode: Text.Wrap
                    color: ThemeValues.color(sheet.looks.text_color ?? "#000000")
                    font.family: sheet.fonts.dialog_text?.family ?? sheet.fonts.interface.family
                    font.pixelSize: sheet.fonts.dialog_text?.size ?? 11
                }
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: sheet.looks.button_inset?.[0] ?? 14
                anchors.bottom: parent.bottom
                anchors.bottomMargin: sheet.looks.button_inset?.[1] ?? 12
                spacing: 0
                Repeater {
                    model: sheet.buttons
                    PushButton {
                        required property string modelData
                        required property int index
                        width: wantedWidth
                        looks: sheet.looks
                        fonts: sheet.fonts
                        themeDirectory: sheet.settings.theme_directory
                        text: modelData
                        isDefault: index === sheet.buttons.length - 1
                        onClicked: sheet.choose(index)
                    }
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#50000000"
            }
        }
    }
}
