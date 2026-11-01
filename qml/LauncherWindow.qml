// what starting the program by hand shows: Grab's window with every choice in one place

import QtQuick
import QtQuick.Window
import Qt.labs.platform as Platform
import Clippings
import "shell" as Shell
import "Look.js" as Look

Window {
    id: launcher

    required property var shell

    title: "New Screen Capture"
    visible: app.launcherShown && !app.ownWindowsHidden
    width: panel.implicitWidth
    height: panel.implicitHeight
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    color: Look.background(settings)

    property int mode: CaptureSession.Selection

    onClosing: app.closed_launcher()

    Platform.FileDialog {
        id: openDialog
        fileMode: Platform.FileDialog.OpenFiles
        title: "Open"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.tif *.tiff *.webp *.bmp *.gif)"]
        onAccepted: app.open_urls(files)
    }

    QtObject {
        id: commands
        property var about: Shell.AppAction { text: "About Clippings"; onTriggered: launcher.shell.about.wanted = true }
        property var preferences: Shell.AppAction { text: "Preferences…"; shortcut: "Ctrl+,"; onTriggered: launcher.shell.preferences.wanted = true }
        property var openFile: Shell.AppAction { text: "Open…"; shortcut: "Ctrl+O"; onTriggered: openDialog.open() }
        property var closeWindow: Shell.AppAction { text: "Close"; shortcut: "Ctrl+W"; onTriggered: launcher.close() }
        property var captureSelection: Shell.AppAction { text: "Selection"; shortcut: "Ctrl+Shift+4"; onTriggered: app.capture(CaptureSession.Selection, true) }
        property var captureWindow: Shell.AppAction { text: "Window"; shortcut: "Ctrl+Shift+5"; onTriggered: app.capture(CaptureSession.Window, true) }
        property var captureScreen: Shell.AppAction { text: "Screen"; shortcut: "Ctrl+Shift+3"; onTriggered: app.capture(CaptureSession.Screen, true) }
        property var captureAll: Shell.AppAction { text: "All Screens"; shortcut: "Ctrl+Shift+6"; onTriggered: app.capture(CaptureSession.AllScreens, true) }
        property var help: Shell.AppAction { text: "Clippings Help"; onTriggered: launcher.shell.help.wanted = true }
    }

    Shell.WindowMenuBar {
        target: launcher
        Platform.Menu {
            title: "Clippings"
            Shell.ActionItem { action: commands.about }
            Platform.MenuSeparator {}
            Shell.ActionItem { action: commands.preferences }
        }
        Platform.Menu {
            title: "File"
            Shell.ActionItem { action: commands.openFile }
            Shell.ActionItem { action: commands.closeWindow }
        }
        Platform.Menu {
            title: "Capture"
            Shell.ActionItem { action: commands.captureSelection }
            Shell.ActionItem { action: commands.captureWindow }
            Shell.ActionItem { action: commands.captureScreen }
            Shell.ActionItem { action: commands.captureAll }
        }
        Platform.Menu {
            title: "Help"
            Shell.ActionItem { action: commands.help }
        }
    }

    Item {
        id: host
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: launcher.close()
        Keys.onReturnPressed: app.capture(launcher.mode, true)

        CapturePanel {
            id: panel
            anchors.fill: parent
            popupHost: host
            mode: launcher.mode
            delay: session.delay
            pointer: session.pointer
            captureLabel: "Start Capture"
            message: "Choose what to capture. This window will not be included in the picture."
            onModeChosen: mode => launcher.mode = mode
            onDelayChosen: seconds => session.delay = seconds
            onPointerChosen: shown => session.pointer = shown
            onCaptureChosen: app.capture(launcher.mode, true)
            onCancelChosen: launcher.close()
        }
    }
}
