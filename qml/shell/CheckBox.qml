// a check box drawn from [controls.checkbox]

import QtQuick
import "ThemeValues.js" as ThemeValues

Item {
    id: box

    required property var settings
    property string text: ""
    property bool checked: false

    signal toggled(bool checked)

    readonly property var look: settings.theme.controls?.checkbox ?? ({})
    readonly property var shape: checked ? look.on : look.off
    readonly property string picture: shape ? (!enabled ? shape.disabled : area.pressed ? shape.pressed : shape.normal) : ""

    implicitWidth: mark.width + 5 + label.implicitWidth
    implicitHeight: 18
    activeFocusOnTab: true
    Accessible.role: Accessible.CheckBox
    Accessible.name: text
    Accessible.checked: checked
    Keys.onSpacePressed: toggled(!checked)

    Image {
        id: mark
        visible: box.picture !== ""
        width: 18
        height: 18
        anchors.verticalCenter: parent.verticalCenter
        source: visible ? "file://" + box.settings.theme_directory + "/" + box.picture : ""
    }
    Rectangle {
        visible: box.picture === ""
        width: 14
        height: 14
        anchors.verticalCenter: parent.verticalCenter
        x: 2
        radius: 3
        border.color: "#8a8a8a"
        color: box.checked ? "#5199db" : "#ffffff"
    }
    Text {
        id: label
        x: mark.width + 5
        anchors.verticalCenter: parent.verticalCenter
        text: box.text
        color: ThemeValues.color(box.enabled ? (box.settings.theme.controls?.text_color ?? "#000000") : "#00000066")
        font.family: box.settings.theme.fonts.interface.family
        font.pixelSize: box.settings.theme.fonts.interface.size
    }
    Rectangle {
        anchors.fill: mark
        radius: 4
        visible: box.activeFocus
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(0.32, 0.6, 0.86, 0.7)
    }
    MouseArea {
        id: area
        anchors.fill: parent
        onClicked: box.toggled(!box.checked)
    }
}
