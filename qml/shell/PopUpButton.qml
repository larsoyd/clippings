// a pop-up button drawn from [controls.popup_button], its list opens in popupHost

import QtQuick
import QtQuick.Window
import "ThemeValues.js" as ThemeValues

Item {
    id: button

    required property var settings
    // labels, in order
    required property var options
    property int current: 0
    property string title: ""
    // the window's content item, the list needs room outside the button
    required property Item popupHost

    signal activated(int index)

    readonly property var look: settings.theme.controls?.popup_button ?? ({})
    readonly property bool open: list.visible

    implicitHeight: 22
    implicitWidth: Math.max(80, label.implicitWidth + (look.text_inset ?? 12) + 26)
    activeFocusOnTab: true
    Accessible.role: Accessible.ComboBox
    Accessible.name: title
    Keys.onSpacePressed: openList()

    function openList() {
        const at = button.mapToItem(popupHost, 0, button.height)
        list.parent = popupHost
        list.openAt = at
        list.returnFocusTo = button.Window.activeFocusItem
        list.opened = true
        list.forceActiveFocus()
    }

    Fill {
        anchors.fill: parent
        themeDirectory: button.settings.theme_directory
        fill: {
            const picture = !button.enabled ? button.look.disabled : (area.pressed ? button.look.pressed : button.look.normal)
            return picture !== undefined ? ({ image: picture, slice: button.look.slice })
                                         : ({ gradient: ["#fbfbfb", "#d8d8d8"], radius: 4, border: { color: "#8a8a8a", width: 1 } })
        }
    }

    Text {
        id: label
        x: button.look.text_inset ?? 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -1
        text: button.options[button.current] ?? ""
        color: ThemeValues.color(button.enabled ? (button.settings.theme.controls?.text_color ?? "#000000") : "#00000066")
        font.family: button.settings.theme.fonts.interface.family
        font.pixelSize: button.settings.theme.fonts.interface.size
    }

    Text {
        visible: !(button.look.includes_arrows ?? false)
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        text: "▾"
        color: ThemeValues.color(button.settings.theme.controls?.text_color ?? "#000000")
    }

    MouseArea {
        id: area
        anchors.fill: parent
        onClicked: button.openList()
    }

    PopUpList {
        id: list
        settings: button.settings
        current: button.current
        options: button.options
        onChosenIndex: index => button.activated(index)
    }
}
