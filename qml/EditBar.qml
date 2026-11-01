// the second bar: tools on the left, their style on the right

import QtQuick
import QtQuick.Window
import "shell" as Shell

Item {
    id: bar

    required property var document
    required property var canvas
    required property Item popupHost
    required property string art

    readonly property var desktop: settings
    readonly property int tool: document.tool
    readonly property bool cropping: tool === 1

    Component {
        id: toolAction
        Shell.AppAction {
            property int kind: 0
            checkable: true
            checked: bar.tool === kind
            onTriggered: bar.document.tool = kind
        }
    }

    readonly property var tools: [
        ["Select", "TB_selection", "Select", 0], ["Crop", "TB_crop", "Crop", 1], ["Arrow", "", "↗", 2],
        ["Line", "TB_annotArrowTemplate", "╱", 3], ["Rectangle", "TB_annotRectangleTemplate", "▭", 4],
        ["Oval", "TB_annotEllipseTemplate", "◯", 5], ["Text", "TB_annotTextPlainTemplate", "A", 6], ["Sketch", "TB_edit", "✎", 7],
        ["Highlight", "TB_annotMarkupHighlightTemplate", "▒", 8], ["Redact", "TB_annotColorWellBase", "■", 9],
        ["Pixelate", "", "▦", 10], ["Blur", "", "◍", 11]
    ].map(entry => toolAction.createObject(bar, { text: entry[0], glyph: entry[1], shortLabel: entry[2], kind: entry[3] }))

    readonly property var widths: [1, 2, 4, 6, 10, 16]
    readonly property var sizes: [12, 16, 24, 36, 48, 72]

    Shell.ToolBarFill {
        anchors.fill: parent
        theme: bar.desktop.theme
        themeDirectory: bar.desktop.theme_directory
        lower: true
    }

    Row {
        x: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        Shell.SegmentedButtons {
            theme: bar.desktop.theme
            themeDirectory: bar.desktop.theme_directory
            artFolder: bar.art
            actions: bar.tools
            segmentWidth: 32
        }
        Shell.PushButton {
            visible: bar.cropping
            width: wantedWidth
            height: 23
            looks: bar.desktop.theme.dialog ?? ({})
            fonts: bar.desktop.theme.fonts
            themeDirectory: bar.desktop.theme_directory
            text: "Crop"
            enabled: bar.canvas.cropDraft.width > 0
            onClicked: bar.canvas.apply_crop()
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        Shell.PopUpButton {
            width: 66
            settings: bar.desktop
            title: "Line thickness"
            popupHost: bar.popupHost
            options: bar.widths.map(value => value + " pt")
            current: bar.widths.indexOf(bar.document.lineWidth)
            onActivated: index => bar.document.lineWidth = bar.widths[index]
        }
        Shell.ColorWell {
            theme: bar.desktop.theme
            themeDirectory: bar.desktop.theme_directory
            popupHost: bar.popupHost
            title: "Line color"
            color: bar.document.strokeColor
            onPicked: color => bar.document.strokeColor = color
        }
        Shell.ColorWell {
            theme: bar.desktop.theme
            themeDirectory: bar.desktop.theme_directory
            popupHost: bar.popupHost
            title: "Fill color"
            allowsNone: true
            color: bar.document.fillColor
            onPicked: color => bar.document.fillColor = color
        }
        Shell.PopUpButton {
            width: 66
            settings: bar.desktop
            title: "Text size"
            popupHost: bar.popupHost
            options: bar.sizes.map(value => value + " pt")
            current: bar.sizes.indexOf(bar.document.fontSize)
            onActivated: index => bar.document.fontSize = bar.sizes[index]
        }
    }
}
