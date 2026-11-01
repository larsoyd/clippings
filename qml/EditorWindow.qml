// one document: a title bar joined to a tool bar, an optional edit bar, and the picture

import QtQuick
import QtQuick.Window
import Qt.labs.platform as Platform
import Clippings
import "shell" as Shell
import "Look.js" as Look

Window {
    id: editor

    required property var document
    required property var shell

    readonly property var desktop: settings
    readonly property var theme: settings.theme
    readonly property string themeDirectory: settings.theme_directory
    readonly property string art: app.artFolder + "/glyphs"
    readonly property int barHeight: Look.value(settings, "tool_bar_height", 38)
    readonly property int editBarHeight: Look.value(settings, "edit_bar_height", 33)

    property bool editBarShown: true
    property var afterSave: null
    property var pendingClose: null
    // nobody cancels a dialog this fast, so a refusal sooner means it never opened
    readonly property int chooserFailMs: 400
    property double chooserOpened: 0

    title: document.title + (document.modified && document.path !== "" ? " — Edited" : "")
    visible: !app.ownWindowsHidden
    color: "transparent"
    minimumWidth: 400
    minimumHeight: 200
    Component.onCompleted: {
        const scale = document.density
        const screenWidth = Screen.width > 0 ? Screen.width : 1280
        const screenHeight = Screen.height > 0 ? Screen.height : 800
        width = Math.max(640, Math.min(document.imageSize.width / scale, screenWidth * 0.8))
        height = Math.min(document.imageSize.height / scale, screenHeight * 0.75) + barHeight + editBarHeight
        document.fontFamily = Look.value(settings, "text_font", "Helvetica")
        canvas.forceActiveFocus()
    }

    onClosing: close => {
        if (document.modified) {
            close.accepted = false
            requestAccess(() => app.close_document(document))
            return
        }
        app.close_document(document)
    }

    // the sheet decides, then the action runs
    function requestAccess(action) {
        pendingClose = action
        raiseSelf()
        sheet.open("Do you want to save the changes made to the document “" + document.title + "”?",
                   "Your changes will be lost if you don’t save them.",
                   ["Don’t Save", "Cancel", "Save"])
    }

    function raiseSelf() {
        raise()
        requestActivate()
    }

    function report(problem) {
        if (problem !== "") {
            failure.open("The document “" + document.title + "” could not be saved.", problem, ["OK"])
        }
    }

    function save() {
        if (document.path !== "") {
            finishSave(document.path)
        } else {
            saveAs()
        }
    }

    function saveAs() {
        console.log("[editor] save as", app.default_save_url(document))
        saveDialog.currentFile = app.default_save_url(document)
        chooserOpened = Date.now()
        saveDialog.open()
        if (!saveDialog.visible) {
            askForPath("no-dialog", "The file chooser isn’t available.")
        }
    }

    function askForPath(reason, words) {
        const path = app.file_of(app.default_save_url(document))
        console.log("[editor] no file chooser reason=" + reason, "path=" + path)
        pathSheet.open(path, words, "Type the full path to save the picture to. The ending picks the format.")
    }

    function copyPicture() {
        const problem = app.copy(document)
        if (problem !== "") {
            failure.open("The picture could not be copied.", problem, ["OK"])
            return
        }
        confirmation.restart()
    }

    function finishSave(path) {
        const problem = document.save_to(path)
        report(problem)
        if (problem === "" && afterSave !== null) {
            const next = afterSave
            afterSave = null
            next()
        }
        if (problem !== "") {
            afterSave = null
        }
    }

    Platform.FileDialog {
        id: saveDialog
        fileMode: Platform.FileDialog.SaveFile
        title: "Save"
        nameFilters: ["PNG (*.png)", "JPEG (*.jpg *.jpeg)", "TIFF (*.tif *.tiff)", "WebP (*.webp)"]
        onAccepted: editor.finishSave(app.file_of(file))
        onRejected: {
            if (Date.now() - editor.chooserOpened < editor.chooserFailMs) {
                editor.askForPath("refused", "The file chooser couldn’t open.")
                return
            }
            editor.afterSave = null
        }
    }
    Platform.FileDialog {
        id: openDialog
        fileMode: Platform.FileDialog.OpenFiles
        title: "Open"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.tif *.tiff *.webp *.bmp *.gif)"]
        onAccepted: app.open_urls(files)
    }

    QtObject {
        id: commands

        property var about: Shell.AppAction { text: "About Clippings"; onTriggered: editor.shell.about.wanted = true }
        property var preferences: Shell.AppAction { text: "Preferences…"; shortcut: "Ctrl+,"; onTriggered: editor.shell.preferences.wanted = true }

        property var openFile: Shell.AppAction { text: "Open…"; shortcut: "Ctrl+O"; onTriggered: openDialog.open() }
        property var closeWindow: Shell.AppAction { text: "Close"; shortcut: "Ctrl+W"; onTriggered: editor.close() }
        property var save: Shell.AppAction { text: "Save"; shortcut: "Ctrl+S"; shortLabel: "Save"; onTriggered: editor.save() }
        property var saveAs: Shell.AppAction { text: "Save As…"; shortcut: "Ctrl+Shift+S"; shortLabel: "Save As…"; icon: "document-save-as"; onTriggered: editor.saveAs() }
        property var saveToPath: Shell.AppAction {
            text: "Save to a Path…"
            shortcut: "Ctrl+Alt+Shift+S"
            onTriggered: editor.askForPath("asked", "Save to a path")
        }
        property var saveAndCopy: Shell.AppAction {
            text: "Save and Copy"
            shortcut: "Ctrl+Alt+S"
            onTriggered: {
                editor.afterSave = () => editor.copyPicture()
                editor.save()
            }
        }

        property var undo: Shell.AppAction { text: "Undo"; shortcut: "Ctrl+Z"; enabled: editor.document.canUndo; onTriggered: editor.document.undo() }
        property var redo: Shell.AppAction { text: "Redo"; shortcut: "Ctrl+Shift+Z"; enabled: editor.document.canRedo; onTriggered: editor.document.redo() }
        property var copy: Shell.AppAction { text: "Copy"; shortcut: "Ctrl+C"; shortLabel: "Copy"; icon: "edit-copy"; onTriggered: editor.copyPicture() }
        property var deleteSelected: Shell.AppAction { text: "Delete"; enabled: editor.document.selected >= 0; onTriggered: editor.document.remove_selected() }

        property var captureSelection: Shell.AppAction { text: "Selection"; shortcut: "Ctrl+Shift+4"; onTriggered: app.capture(CaptureSession.Selection, true) }
        property var captureWindow: Shell.AppAction { text: "Window"; shortcut: "Ctrl+Shift+5"; onTriggered: app.capture(CaptureSession.Window, true) }
        property var captureScreen: Shell.AppAction { text: "Screen"; shortcut: "Ctrl+Shift+3"; onTriggered: app.capture(CaptureSession.Screen, true) }
        property var captureAll: Shell.AppAction { text: "All Screens"; shortcut: "Ctrl+Shift+6"; onTriggered: app.capture(CaptureSession.AllScreens, true) }

        property var actualSize: Shell.AppAction { text: "Actual Size"; shortcut: "Ctrl+0"; glyph: "TB_actualSize"; shortLabel: "1:1"; onTriggered: canvas.actual_size() }
        property var zoomFit: Shell.AppAction { text: "Zoom to Fit"; shortcut: "Ctrl+9"; glyph: "TB_fitPopup"; shortLabel: "Fit"; onTriggered: canvas.fits = true }
        property var zoomIn: Shell.AppAction { text: "Zoom In"; shortcut: "Ctrl++"; glyph: "TB_zoomIn"; shortLabel: "+"; onTriggered: canvas.zoom_by(1.25) }
        property var zoomOut: Shell.AppAction { text: "Zoom Out"; shortcut: "Ctrl+-"; glyph: "TB_zoomOut"; shortLabel: "−"; onTriggered: canvas.zoom_by(0.8) }
        property var editToolbar: Shell.AppAction {
            text: editor.editBarShown ? "Hide Edit Toolbar" : "Show Edit Toolbar"
            shortcut: "Ctrl+Shift+A"
            glyph: "TB_edit"
            shortLabel: "Edit"
            checked: editor.editBarShown
            onTriggered: editor.editBarShown = !editor.editBarShown
        }
        property var inspector: Shell.AppAction {
            text: "Show Inspector"
            shortcut: "Ctrl+I"
            glyph: "InspectorGeneralInfo"
            shortLabel: "Info"
            onTriggered: inspectorWindow.wanted = !inspectorWindow.wanted
        }

        property var minimize: Shell.AppAction { text: "Minimize"; shortcut: "Ctrl+M"; onTriggered: app.minimize_window(editor) }
        property var zoomWindow: Shell.AppAction { text: "Zoom"; onTriggered: editor.visibility === Window.Maximized ? editor.showNormal() : editor.showMaximized() }
        property var bringAll: Shell.AppAction { text: "Bring All to Front"; onTriggered: app.bring_forward() }
        property var help: Shell.AppAction { text: "Clippings Help"; onTriggered: editor.shell.help.wanted = true }
    }

    AppMenus {
        target: editor
        commands: commands
    }

    Item {
        id: content
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: Look.background(settings)
        }

        Shell.ToolBarFill {
            id: toolBar
            width: parent.width
            height: editor.barHeight
            theme: editor.theme
            themeDirectory: editor.themeDirectory

            Row {
                x: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Rectangle {
                    id: dragWell
                    width: 30
                    height: 23
                    color: "transparent"
                    Accessible.role: Accessible.Graphic
                    Accessible.name: "Drag this picture to another program"
                    Image {
                        anchors.fill: parent
                        source: app.iconSource
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(46, 46)
                    }
                    MouseArea {
                        anchors.fill: parent
                        property point start
                        onPressed: mouse => start = Qt.point(mouse.x, mouse.y)
                        onPositionChanged: mouse => {
                            if (pressed && Math.abs(mouse.x - start.x) + Math.abs(mouse.y - start.y) > 6) {
                                app.drag_out(editor.document, dragWell)
                            }
                        }
                    }
                }
                Shell.SegmentedButtons {
                    theme: editor.theme
                    themeDirectory: editor.themeDirectory
                    artFolder: editor.art
                    actions: [commands.zoomOut, commands.zoomIn]
                }
                Shell.SegmentedButtons {
                    theme: editor.theme
                    themeDirectory: editor.themeDirectory
                    artFolder: editor.art
                    actions: [commands.actualSize, commands.zoomFit]
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Text {
                    id: copied
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Copied"
                    opacity: 0
                    color: "#404040"
                    font.family: editor.theme.fonts.interface.family
                    font.pixelSize: 11
                    SequentialAnimation {
                        id: confirmation
                        PropertyAction { target: copied; property: "opacity"; value: 1 }
                        PauseAnimation { duration: 1400 }
                        NumberAnimation { target: copied; property: "opacity"; to: 0; duration: 300 }
                    }
                }
                Shell.ToolButton {
                    theme: editor.theme
                    themeDirectory: editor.themeDirectory
                    artFolder: editor.art
                    action: commands.copy
                }
                Shell.ToolButton {
                    theme: editor.theme
                    themeDirectory: editor.themeDirectory
                    artFolder: editor.art
                    action: commands.saveAs
                }
                Shell.ToolButton {
                    theme: editor.theme
                    themeDirectory: editor.themeDirectory
                    artFolder: editor.art
                    action: commands.inspector
                }
                Shell.ToolButton {
                    theme: editor.theme
                    themeDirectory: editor.themeDirectory
                    artFolder: editor.art
                    action: commands.editToolbar
                }
            }
        }

        EditBar {
            id: editBar
            y: toolBar.height
            width: parent.width
            height: editor.editBarShown ? editor.editBarHeight : 0
            visible: height > 0
            document: editor.document
            canvas: canvas
            popupHost: content
            art: editor.art
        }

        Item {
            id: pictureArea
            anchors.top: editBar.bottom
            anchors.bottom: parent.bottom
            width: parent.width
            clip: true

            DocumentCanvas {
                id: canvas
                anchors.fill: parent
                document: editor.document
                Accessible.role: Accessible.Graphic
                Accessible.name: "Picture, " + editor.document.imageSize.width + " by " + editor.document.imageSize.height + " pixels"
                activeFocusOnTab: true

                Keys.onPressed: event => {
                    const step = event.modifiers & Qt.ShiftModifier ? 10 : 1
                    if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
                        editor.document.remove_selected()
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        canvas.apply_crop()
                    } else if (event.key === Qt.Key_Escape) {
                        canvas.clear_crop()
                        editor.document.selected = -1
                        editor.document.tool = 0
                    } else if (event.key === Qt.Key_Left) {
                        canvas.nudge_selected(-step, 0)
                    } else if (event.key === Qt.Key_Right) {
                        canvas.nudge_selected(step, 0)
                    } else if (event.key === Qt.Key_Up) {
                        canvas.nudge_selected(0, -step)
                    } else if (event.key === Qt.Key_Down) {
                        canvas.nudge_selected(0, step)
                    } else {
                        return
                    }
                    event.accepted = true
                }

                onTextRequested: (x, y, index) => textEntry.begin(x, y, index)
            }

            Shell.ScrollKnob {
                horizontal: false
                visibleShare: canvas.height / Math.max(1, canvas.viewHeight)
                position: canvas.scrollY / Math.max(1, canvas.viewHeight - canvas.height)
            }
            Shell.ScrollKnob {
                horizontal: true
                visibleShare: canvas.width / Math.max(1, canvas.viewWidth)
                position: canvas.scrollX / Math.max(1, canvas.viewWidth - canvas.width)
            }

            TextEntry {
                id: textEntry
                canvas: canvas
                document: editor.document
            }
        }

        Shell.AlertSheet {
            id: sheet
            cancelIndex: 1
            settings: editor.desktop
            iconSource: app.iconSource
            topInset: toolBar.height
            onChosen: index => {
                const action = editor.pendingClose
                editor.pendingClose = null
                if (index === 0 && action !== null) {
                    editor.document.discard()
                    action()
                } else if (index === 2 && action !== null) {
                    editor.afterSave = action
                    editor.save()
                }
            }
        }
        Shell.PathSheet {
            id: pathSheet
            settings: editor.desktop
            topInset: toolBar.height
            onAccepted: path => editor.finishSave(path)
            onCancelled: editor.afterSave = null
        }

        Shell.AlertSheet {
            id: failure
            settings: editor.desktop
            iconSource: app.iconSource
            topInset: toolBar.height
        }
    }

    InspectorWindow {
        id: inspectorWindow
        document: editor.document
        transientParent: editor
    }
}
