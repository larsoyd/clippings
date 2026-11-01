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
}
