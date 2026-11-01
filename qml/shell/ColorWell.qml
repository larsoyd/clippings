// a colour well: a tool button holding a swatch, opening a grid of swatches in popupHost

import QtQuick
import QtQuick.Window

Item {
    id: well

    required property var theme
    required property string themeDirectory
    required property Item popupHost
    property color color: "black"
    property string title: ""
    property bool allowsNone: false

    signal picked(color color)

    readonly property var swatches: ["#000000", "#ffffff", "#808080", "#d0211c", "#f08a24", "#f7d51d", "#3dae2b", "#1d7fe0", "#7a3fc8", "#e0449a"]
    readonly property var look: theme.controls?.tool_button ?? ({})

    implicitWidth: 36
    implicitHeight: 23
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: title
    Keys.onSpacePressed: openGrid()

    function openGrid() {
        grid.parent = popupHost
        const at = well.mapToItem(popupHost, 0, well.height + 2)
        panel.x = Math.max(0, Math.min(at.x, popupHost.width - panel.width))
        panel.y = at.y
        grid.visible = true
        grid.forceActiveFocus()
    }

    Fill {
        anchors.fill: parent
        themeDirectory: well.themeDirectory
        opacity: Window.active ? 1 : 0.6
        fill: well.look.normal !== undefined ? ({ image: well.look.normal, slice: well.look.slice })
                                             : ({ gradient: ["#fbfbfb", "#d8d8d8"], radius: 4, border: { color: "#8a8a8a", width: 1 } })
    }
    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 6
        anchors.bottomMargin: 7
        color: well.color
        border.width: 1
        border.color: "#80000000"
        Rectangle {
            visible: well.color.a === 0
            anchors.centerIn: parent
            width: parent.width + 2
            height: 1.5
            rotation: -22
            color: "#d0211c"
        }
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: 5
        visible: well.activeFocus
        color: "transparent"
        border.width: 2
        border.color: Qt.rgba(0.32, 0.6, 0.86, 0.7)
    }
    MouseArea {
        anchors.fill: parent
        onClicked: well.openGrid()
    }

    Item {
        id: grid
        anchors.fill: parent
        visible: false
        z: 1000
        Keys.onEscapePressed: visible = false

        MouseArea {
            anchors.fill: parent
            onPressed: grid.visible = false
        }
        Rectangle {
            id: panel
            width: 136
            height: well.allowsNone ? 84 : 60
            radius: 5
            color: "#f4f4f4"
            border.color: "#40000000"
            Grid {
                x: 6
                y: 6
                columns: 5
                spacing: 2
                Repeater {
                    model: well.swatches
                    Rectangle {
                        required property string modelData
                        width: 23
                        height: 22
                        color: modelData
                        border.color: "#60000000"
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                grid.visible = false
                                well.picked(parent.color)
                            }
                        }
                    }
                }
            }
            Text {
                visible: well.allowsNone
                x: 8
                y: 59
                text: "No Fill"
                font.family: well.theme.fonts.interface.family
                font.pixelSize: well.theme.fonts.interface.size
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        grid.visible = false
                        well.picked("transparent")
                    }
                }
            }
        }
    }
}
