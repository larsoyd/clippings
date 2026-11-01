// the menus of one window, each entry bound to a shared action

import QtQuick
import Qt.labs.platform as Platform
import "shell" as Shell

Shell.WindowMenuBar {
    id: bar

    required property var commands
    readonly property var c: commands

    Platform.Menu {
        title: "Clippings"
        Shell.ActionItem { action: bar.c.about }
        Platform.MenuSeparator {}
        Shell.ActionItem { action: bar.c.preferences }
    }
    Platform.Menu {
        title: "File"
        Shell.ActionItem { action: bar.c.openFile }
        Platform.MenuSeparator {}
        Shell.ActionItem { action: bar.c.closeWindow }
        Shell.ActionItem { action: bar.c.save }
        Shell.ActionItem { action: bar.c.saveAs }
        Shell.ActionItem { action: bar.c.saveAndCopy }
    }
    Platform.Menu {
        title: "Edit"
        Shell.ActionItem { action: bar.c.undo }
        Shell.ActionItem { action: bar.c.redo }
        Platform.MenuSeparator {}
        Shell.ActionItem { action: bar.c.copy }
        Shell.ActionItem { action: bar.c.deleteSelected }
    }
    Platform.Menu {
        title: "Capture"
        Shell.ActionItem { action: bar.c.captureSelection }
        Shell.ActionItem { action: bar.c.captureWindow }
        Shell.ActionItem { action: bar.c.captureScreen }
        Shell.ActionItem { action: bar.c.captureAll }
    }
    Platform.Menu {
        title: "View"
        Shell.ActionItem { action: bar.c.actualSize }
        Shell.ActionItem { action: bar.c.zoomFit }
        Shell.ActionItem { action: bar.c.zoomIn }
        Shell.ActionItem { action: bar.c.zoomOut }
        Platform.MenuSeparator {}
        Shell.ActionItem { action: bar.c.editToolbar }
        Shell.ActionItem { action: bar.c.inspector }
    }
    Platform.Menu {
        title: "Window"
        Shell.ActionItem { action: bar.c.minimize }
        Shell.ActionItem { action: bar.c.zoomWindow }
        Platform.MenuSeparator {}
        Shell.ActionItem { action: bar.c.bringAll }
    }
    Platform.Menu {
        title: "Help"
        Shell.ActionItem { action: bar.c.help }
    }
}
