// what the program shows: a window per document, and the panels they share

import QtQuick
import QtQuick.Window

QtObject {
    id: root

    property Instantiator editors: Instantiator {
        model: app.documents
        delegate: EditorWindow {
            shell: root
        }
    }

    property LauncherWindow launcher: LauncherWindow {
        shell: root
    }
    property PreferencesWindow preferences: PreferencesWindow {}
    property AboutWindow about: AboutWindow {}
    property HelpWindow help: HelpWindow {}
    property AlertWindow alerts: AlertWindow {}

    // closes each window in turn, an unsaved document's sheet can stop it
    function quitAll() {
        for (let index = editors.count - 1; index >= 0; index--) {
            const editor = editors.objectAt(index)
            if (editor !== null && editor.document !== null && editor.document.modified) {
                editor.requestAccess(() => quitAll())
                return
            }
        }
        app.quit_now()
    }

    property Connections quitting: Connections {
        target: app
        function onQuitRequested() {
            root.quitAll()
        }
    }
}
