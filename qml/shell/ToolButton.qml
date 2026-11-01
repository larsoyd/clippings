// a tool bar button drawn from the theme's [controls.tool_button]

import QtQuick
import QtQuick.Window

Item {
    id: button

    required property var theme
    required property string themeDirectory
    required property string artFolder
    property var action: null
    property string glyph: action?.glyph ?? ""
    property string label: action?.shortLabel ?? ""
    property string tip: action?.text ?? ""
    property bool checked: action?.checked ?? false
    enabled: action?.enabled ?? true

    signal clicked()

    readonly property var look: theme.controls?.tool_button ?? ({})
    readonly property bool held: area.pressed && area.containsMouse

    implicitWidth: Math.max(36, shape.implicitWidth + 16)
    implicitHeight: 23
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: tip
    Keys.onSpacePressed: activate()
    Keys.onReturnPressed: activate()

    function activate() {
        if (action) {
            action.trigger()
        }
        clicked()
    }

    Fill {
        anchors.fill: parent
        themeDirectory: button.themeDirectory
        opacity: Window.active ? 1 : 0.6
        fill: {
            const picture = !button.enabled ? (button.look.disabled ?? button.look.normal)
                : (button.held || button.checked) ? (button.look.pressed ?? button.look.normal)
                : button.look.normal
            return picture !== undefined ? ({ image: picture, slice: button.look.slice })
                                         : ({ gradient: button.held || button.checked ? ["#c4c4c4", "#d8d8d8"] : ["#fbfbfb", "#d8d8d8"],
                                              radius: 4, border: { color: "#8a8a8a", width: 1 } })
        }
    }

    Glyph {
        id: shape
        anchors.centerIn: parent
        folder: button.artFolder
        name: button.glyph
        fallback: button.label
        labelFont: button.theme.fonts.interface
        side: 16
        inkOpacity: !button.enabled ? 0.3 : Window.active ? 0.8 : 0.5
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: 5
        visible: button.activeFocus
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(0.32, 0.6, 0.86, 0.7)
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: button.tip !== ""
        onClicked: button.activate()
    }
}
