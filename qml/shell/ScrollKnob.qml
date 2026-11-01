// the slim knob that shows while content moves, as scrolling does in the theme's period

import QtQuick

Rectangle {
    id: knob

    // share of the content that is visible, and how far along it is
    required property real visibleShare
    required property real position
    required property bool horizontal

    readonly property real room: (horizontal ? parent.width : parent.height) - 6
    readonly property real along: Math.max(24, room * visibleShare)
    x: horizontal ? 3 + position * (room - along) : parent.width - width - 2
    y: horizontal ? parent.height - height - 2 : 3 + position * (room - along)
    width: horizontal ? along : 7
    height: horizontal ? 7 : along
    radius: 3.5
    color: "#80000000"
    border.color: "#40ffffff"
    visible: visibleShare < 1
}
