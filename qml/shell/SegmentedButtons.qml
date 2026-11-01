// joined tool bar buttons: every segment shows its part of one shared picture, so the caps stay at the ends

import QtQuick
import QtQuick.Window
import "ThemeValues.js" as ThemeValues

Item {
    id: control

    required property var theme
    required property string themeDirectory
    required property string artFolder
    // actions, or objects shaped like them
    required property var actions
    property int segmentWidth: 34
    // a layer surface never counts as active
    property bool alwaysActive: false
    readonly property bool lit: alwaysActive || Window.active

    readonly property var look: theme.controls?.tool_button ?? ({})

    implicitWidth: actions.length * segmentWidth
    implicitHeight: 23

    Repeater {
        model: control.actions

        Item {
            id: segment
            required property var modelData
            required property int index

            readonly property bool held: area.pressed && area.containsMouse

            x: index * control.segmentWidth
            width: control.segmentWidth
            height: control.height
            clip: true
            enabled: modelData.enabled
            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: modelData.text
            Accessible.checked: modelData.checked
            Accessible.checkable: modelData.checkable
            Keys.onSpacePressed: modelData.trigger()

            Fill {
                x: -segment.x
                width: control.width
                height: control.height
                themeDirectory: control.themeDirectory
                opacity: control.lit ? 1 : 0.6
                fill: {
                    const picture = !segment.enabled ? (control.look.disabled ?? control.look.normal)
                        : (segment.held || segment.modelData.checked) ? (control.look.pressed ?? control.look.normal)
                        : control.look.normal
                    return picture !== undefined ? ({ image: picture, slice: control.look.slice })
                                                 : ({ gradient: ["#fbfbfb", "#d8d8d8"], radius: 4, border: { color: "#8a8a8a", width: 1 } })
                }
            }

            Rectangle {
                visible: segment.index > 0
                y: 1
                width: 1
                height: parent.height - 2
                color: ThemeValues.color("#00000055")
            }

            Glyph {
                anchors.centerIn: parent
                folder: control.artFolder
                name: segment.modelData.glyph
                fallback: segment.modelData.shortLabel
                labelFont: control.theme.fonts.interface
                side: 16
                inkOpacity: !segment.enabled ? 0.3 : control.lit ? 0.8 : 0.5
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                visible: segment.activeFocus
                color: "transparent"
                border.width: 1
                border.color: Qt.rgba(0.32, 0.6, 0.86, 0.9)
            }

            MouseArea {
                id: area
                anchors.fill: parent
                onClicked: segment.modelData.trigger()
            }
        }
    }
}
