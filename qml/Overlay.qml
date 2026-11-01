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
        color: overlay.isSelection ? Qt.rgba(0.53, 0.53, 0.53, 0.25) : Qt.rgba(0.53, 0.73, 0.91, 0.5)
        border.width: 1
        border.color: overlay.isSelection ? "#ffffff" : "#b05199db"

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
                color: "#266ed8"
                border.color: "#ffffff"
            }
        }
    }

    // white on a black edge as the original draws it, larger so it reads on today's displays
    Text {
        visible: overlay.pointerHere && overlay.shown
        x: Math.min(overlay.pointerX + 18, overlay.width - width - 4)
        y: Math.min(overlay.pointerY + 18, overlay.height - height - 4)
        text: session.pixelSize.width + " × " + session.pixelSize.height
        color: "#ffffff"
        style: Text.Outline
        styleColor: "#000000"
        font.family: overlay.theme.fonts.interface.family
        font.pixelSize: 12
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        objectName: "pointerArea"
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
            shadow: session.windowShadow
            canCapture: session.target.width > 0
            message: session.mode === CaptureSession.Selection ? "Drag over the portion of the screen you want to capture."
                : session.mode === CaptureSession.Window ? "Click the window you want to capture." + (session.targetTitle !== "" ? "\n“" + session.targetTitle + "”" : "")
                : session.mode === CaptureSession.Screen ? "Click to capture this screen."
                : "Click to capture all of the screens."
            onModeChosen: mode => session.mode = mode
            onDelayChosen: seconds => session.delay = seconds
            onPointerChosen: shown => session.pointer = shown
            onShadowChosen: shown => session.windowShadow = shown
            onCaptureChosen: session.accept()
            onCancelChosen: session.cancel()
        }
        MouseArea {
            anchors.fill: parent
            z: -1
        }
    }
}
