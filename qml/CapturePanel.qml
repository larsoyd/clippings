// the capture window's contents, laid out from Grab's: icon, words, choices, then the two buttons

import QtQuick
import QtQuick.Window
import Clippings
import "shell" as Shell
import "shell/ThemeValues.js" as ThemeValues

Item {
    id: panel

    property int mode: CaptureSession.Selection
    property int delay: 0
    property bool pointer: false
    property string captureLabel: "Capture"
    property bool canCapture: true
    required property Item popupHost
    property string message: ""

    signal modeChosen(int mode)
    signal delayChosen(int seconds)
    signal pointerChosen(bool shown)
    signal captureChosen()
    signal cancelChosen()

    readonly property var desktop: settings
    readonly property var delays: [0, 3, 5, 10]
    readonly property string art: app.artFolder + "/glyphs"

    implicitWidth: 391
    implicitHeight: 208

    Component {
        id: modeAction
        Shell.AppAction {
            property int value: 0
            checkable: true
            checked: panel.mode === value
            onTriggered: panel.modeChosen(value)
        }
    }
    readonly property var modes: [["Selection", CaptureSession.Selection], ["Window", CaptureSession.Window],
                                  ["Screen", CaptureSession.Screen], ["All Screens", CaptureSession.AllScreens]]
        .map(entry => modeAction.createObject(panel, { text: entry[0], shortLabel: entry[0], value: entry[1] }))

    Image {
        x: 12
        y: 12
        width: 64
        height: 64
        source: app.iconSource
        sourceSize: Qt.size(128, 128)
    }
    Text {
        x: 94
        y: 12
        width: 278
        height: 62
        text: panel.message
        wrapMode: Text.Wrap
        color: ThemeValues.color(panel.desktop.theme.controls?.text_color ?? "#000000")
        font.family: panel.desktop.theme.fonts.interface.family
        font.pixelSize: panel.desktop.theme.fonts.interface.size
    }
    Shell.SegmentedButtons {
        x: 15
        y: 86
        width: 360
        theme: panel.desktop.theme
        themeDirectory: panel.desktop.theme_directory
        artFolder: panel.art
        actions: panel.modes
        segmentWidth: 90
        alwaysActive: true
    }
    Row {
        x: 15
        y: 122
        spacing: 12
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "Delay:"
            font.family: panel.desktop.theme.fonts.interface.family
            font.pixelSize: panel.desktop.theme.fonts.interface.size
        }
        Shell.PopUpButton {
            width: 112
            settings: panel.desktop
            title: "Delay"
            popupHost: panel.popupHost
            options: panel.delays.map(seconds => seconds === 0 ? "None" : seconds + " Seconds")
            current: Math.max(0, panel.delays.indexOf(panel.delay))
            onActivated: index => panel.delayChosen(panel.delays[index])
        }
        Shell.CheckBox {
            anchors.verticalCenter: parent.verticalCenter
            settings: panel.desktop
            text: "Show Pointer"
            checked: panel.pointer
            onToggled: checked => panel.pointerChosen(checked)
        }
    }
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        Shell.PushButton {
            width: Math.max(82, wantedWidth)
            looks: panel.desktop.theme.dialog ?? ({})
            fonts: panel.desktop.theme.fonts
            themeDirectory: panel.desktop.theme_directory
            text: "Cancel"
            onClicked: panel.cancelChosen()
        }
        Shell.PushButton {
            width: Math.max(141, wantedWidth)
            looks: panel.desktop.theme.dialog ?? ({})
            fonts: panel.desktop.theme.fonts
            themeDirectory: panel.desktop.theme_directory
            text: panel.captureLabel
            isDefault: true
            enabled: panel.canCapture
            onClicked: panel.captureChosen()
        }
    }
}
