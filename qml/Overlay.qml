// the frozen picture of one display with the shared selection over it

import QtQuick
import QtQuick.Effects
import Clippings
import "shell/ThemeValues.js" as ThemeValues

Item {
    id: overlay

    required property string outputName
    required property int outputX
    required property int outputY
    required property string picture

    readonly property var theme: settings.theme
    readonly property rect target: session.target
    readonly property bool shown: target.width > 0
    readonly property bool hasPointer: pointerX >= 0
    property int pointerX: session.pointerAt.x - outputX
    property int pointerY: session.pointerAt.y - outputY
    readonly property bool pointerHere: pointerX >= 0 && pointerY >= 0 && pointerX < width && pointerY < height
    readonly property bool isSelection: session.mode === CaptureSession.Selection

    focus: true
    Component.onCompleted: forceActiveFocus()

    Keys.onPressed: event => {
        const step = event.modifiers & Qt.ShiftModifier ? 10 : 1
        const resize = (event.modifiers & Qt.AltModifier) !== 0
        switch (event.key) {
        case Qt.Key_Escape:
            session.cancel()
            break
        case Qt.Key_Return:
        case Qt.Key_Enter:
            session.accept()
            break
        case Qt.Key_Left:
            session.nudge(-step, 0, resize)
            break
        case Qt.Key_Right:
            session.nudge(step, 0, resize)
            break
        case Qt.Key_Up:
            session.nudge(0, -step, resize)
            break
        case Qt.Key_Down:
            session.nudge(0, step, resize)
            break
        case Qt.Key_Space:
            session.mode = session.mode === CaptureSession.Window ? CaptureSession.Selection : CaptureSession.Window
            break
        case Qt.Key_Tab:
            session.mode = (session.mode + 1) % 4
            break
        default:
            return
        }
        event.accepted = true
    }

    Image {
        anchors.fill: parent
        source: overlay.picture
        cache: false
        smooth: false
        mipmap: false
    }

    Rectangle {
        id: chosen
        x: overlay.target.x - overlay.outputX
        y: overlay.target.y - overlay.outputY
        width: overlay.target.width
        height: overlay.target.height
        visible: overlay.shown
        color: overlay.isSelection ? "#30ffffff" : Qt.rgba(0.32, 0.6, 0.86, 0.32)
        border.width: 1
        border.color: overlay.isSelection ? "#c0ffffff" : "#b05199db"

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            color: "transparent"
            border.color: "#80000000"
        }
        Repeater {
            model: overlay.isSelection ? [[0, 0], [0.5, 0], [1, 0], [1, 0.5], [1, 1], [0.5, 1], [0, 1], [0, 0.5]] : []
            Rectangle {
                required property var modelData
                x: modelData[0] * (chosen.width - 1) - 4
                y: modelData[1] * (chosen.height - 1) - 4
                width: 9
                height: 9
                radius: 4.5
                color: "#ffffff"
                border.color: "#266ed8"
            }
        }
    }

    Rectangle {
        // the measure stays with the pointer, the way the period's capture cursor carries it
        visible: overlay.pointerHere && overlay.shown
        x: Math.min(overlay.pointerX + 14, overlay.width - width - 4)
        y: Math.min(overlay.pointerY + 14, overlay.height - height - 4)
        width: size.implicitWidth + 12
        height: size.implicitHeight + 4
        radius: 3
        color: "#b0000000"
        Text {
            id: size
            anchors.centerIn: parent
            text: session.pixelSize.width + " × " + session.pixelSize.height
            color: "#ffffff"
            font.family: overlay.theme.fonts.interface.family
            font.pixelSize: 11
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: session.mode === CaptureSession.Selection ? Qt.CrossCursor : Qt.PointingHandCursor
        onPositionChanged: mouse => session.pointer_moved(mouse.x + overlay.outputX, mouse.y + overlay.outputY)
        onPressed: mouse => session.pressed(mouse.x + overlay.outputX, mouse.y + overlay.outputY)
        onReleased: mouse => session.released(mouse.x + overlay.outputX, mouse.y + overlay.outputY)
        onDoubleClicked: session.accept()
    }

    Item {
        id: hud
        visible: overlay.pointerHere || session.pointerOutput === overlay.outputName
        readonly property bool overlapsSelection: overlay.isSelection && overlay.shown
            && chosen.y + chosen.height > overlay.height - height - 60
        anchors.horizontalCenter: parent.horizontalCenter
        y: overlapsSelection ? 60 : overlay.height - height - 60
        width: body.implicitWidth
        height: body.implicitHeight

        RectangularShadow {
            anchors.fill: parent
            radius: 6
            blur: 26
            offset: Qt.vector2d(0, 8)
            color: "#70000000"
        }
        Rectangle {
            anchors.fill: parent
            radius: 6
            color: ThemeValues.color(overlay.theme.dialog?.background ?? "#ededed")
            border.color: "#40000000"
        }
        CapturePanel {
            id: body
            anchors.fill: parent
            popupHost: overlay
            mode: session.mode
            delay: session.delay
            pointer: session.pointer
            canCapture: session.target.width > 0
            message: session.mode === CaptureSession.Selection ? "Drag over the portion of the screen you want to capture."
                : session.mode === CaptureSession.Window ? "Click the window you want to capture." + (session.targetTitle !== "" ? "\n“" + session.targetTitle + "”" : "")
                : session.mode === CaptureSession.Screen ? "Click to capture this screen."
                : "Click to capture all of the screens."
            onModeChosen: mode => session.mode = mode
            onDelayChosen: seconds => session.delay = seconds
            onPointerChosen: shown => session.pointer = shown
            onCaptureChosen: session.accept()
            onCancelChosen: session.cancel()
        }
        MouseArea {
            anchors.fill: parent
            z: -1
        }
    }
}
