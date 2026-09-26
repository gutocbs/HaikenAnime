import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic as Basic
import QtQuick.Dialogs
import QtQuick.Layouts 1.15

Popup {
    id: panel

    required property var controller
    property color ink: "#172033"
    property color muted: "#68758a"
    property color line: "#d7dee8"
    property color surface: "#ffffff"
    property color surfaceSoft: "#f6f8fb"
    property color accent: "#315d91"
    property int editingMediaId: 0
    property int scoreDecimals: controller.scoreStep < 0.1 ? 2 : controller.scoreStep < 1 ? 1 : 0
    property int scoreFactor: Math.pow(10, scoreDecimals)

    signal applyRequested(int mediaId, int progress, string statusKey, real score,
                          string path, string alternativeNames)

    function indexForStatus(key) {
        for (let i = 0; i < statusField.count; ++i) {
            if (statusField.valueAt(i) === key) return i
        }
        return 0
    }

    function openForMedia(mediaId, progress, statusKey, score, path, alternativeNames) {
        editingMediaId = mediaId
        progressField.value = progress
        statusField.currentIndex = indexForStatus(statusKey)
        scoreField.value = Math.round(score * scoreFactor)
        pathField.text = path || ""
        alternativeNamesField.text = alternativeNames || ""
        open()
    }

    parent: Overlay.overlay
    x: 0
    y: 0
    width: Math.min(520, parent ? parent.width : 520)
    height: parent ? parent.height : 720
    modal: true
    dim: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: "#260f1d2e" }
    background: Rectangle {
        color: panel.surface
        border.color: panel.line
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 22
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label { text: qsTr("Editar mídia"); color: panel.ink; font.pixelSize: 20; font.weight: Font.Bold }
                Label { text: panel.controller.selectedTitle; color: panel.muted; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true }
            }

            Button { text: qsTr("Fechar"); flat: true; onClicked: panel.close() }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: panel.line }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: form.implicitHeight + 44
            clip: true

            ColumnLayout {
                id: form
                width: parent.width - 44
                x: 22
                y: 22
                spacing: 18

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 14
                    rowSpacing: 8

                    Label { text: qsTr("Progresso"); color: panel.muted; font.pixelSize: 11 }
                    Label { text: qsTr("Status"); color: panel.muted; font.pixelSize: 11 }

                    Basic.SpinBox {
                        id: progressField
                        Layout.fillWidth: true
                        from: 0
                        to: panel.controller.selectedProgressMaximum > 0
                            ? panel.controller.selectedProgressMaximum : 99999
                        editable: true
                    }

                    Basic.ComboBox {
                        id: statusField
                        Layout.fillWidth: true
                        model: panel.controller.availableListOptions.filter(function(option) {
                            return option.key !== "all"
                        })
                        textRole: "label"
                        valueRole: "key"

                        delegate: Basic.ItemDelegate {
                            required property int index
                            required property var model

                            width: statusField.popup ? statusField.popup.width : statusField.width
                            implicitHeight: 38
                            highlighted: statusField.highlightedIndex === index

                            contentItem: Text {
                                text: model[statusField.textRole]
                                color: panel.ink
                                font.pixelSize: 12
                                font.weight: statusField.currentIndex === index
                                             ? Font.DemiBold : Font.Normal
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                color: parent.highlighted ? "#eaf1f8"
                                                          : statusField.currentIndex === index
                                                            ? "#f4f7fb" : panel.surface
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: (form.width - 14) / 2
                    Layout.maximumWidth: (form.width - 14) / 2
                    Layout.alignment: Qt.AlignLeft
                    spacing: 6
                    Label { text: qsTr("Nota"); color: panel.muted; font.pixelSize: 11 }
                    Basic.SpinBox {
                        id: scoreField
                        Layout.fillWidth: true
                        from: Math.round(panel.controller.scoreMinimum * panel.scoreFactor)
                        to: Math.round(panel.controller.scoreMaximum * panel.scoreFactor)
                        stepSize: Math.max(1, Math.round(panel.controller.scoreStep * panel.scoreFactor))
                        editable: true
                        textFromValue: function(value, locale) {
                            return Number(value / panel.scoreFactor).toLocaleString(locale, "f", panel.scoreDecimals)
                        }
                        valueFromText: function(text, locale) {
                            return Math.round(Number.fromLocaleString(locale, text) * panel.scoreFactor)
                        }
                    }
                    Label {
                        text: qsTr("Escala configurada: %1 a %2 · incremento de %3")
                              .arg(panel.controller.scoreMinimum)
                              .arg(panel.controller.scoreMaximum)
                              .arg(panel.controller.scoreStep)
                        color: panel.muted
                        font.pixelSize: 10
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label { text: qsTr("Pasta da mídia"); color: panel.muted; font.pixelSize: 11 }
                    RowLayout {
                        Layout.fillWidth: true
                        Basic.TextField { id: pathField; Layout.fillWidth: true; placeholderText: qsTr("C:\\Mídias\\Título") }
                        Button { text: qsTr("Selecionar pasta"); onClicked: folderDialog.open() }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label { text: qsTr("Nomes alternativos"); color: panel.muted; font.pixelSize: 11 }
                    Basic.TextArea {
                        id: alternativeNamesField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 118
                        placeholderText: qsTr("Nome em inglês; Nome original; Outro título")
                        wrapMode: TextEdit.Wrap
                        background: Rectangle { color: panel.surfaceSoft; border.color: panel.line; radius: 5 }
                    }
                    Label { text: qsTr("Separe os nomes usando ponto e vírgula (;).") ; color: panel.muted; font.pixelSize: 10 }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: panel.line }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 18
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Cancelar"); flat: true; onClicked: panel.close() }
            Button {
                text: qsTr("Aplicar alterações")
                highlighted: true
                onClicked: {
                    panel.applyRequested(panel.editingMediaId, progressField.value,
                                         statusField.currentValue,
                                         scoreField.value / panel.scoreFactor,
                                         pathField.text.trim(), alternativeNamesField.text.trim())
                    panel.close()
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Selecionar pasta da mídia")
        onAccepted: {
            let value = decodeURIComponent(selectedFolder.toString())
            value = value.replace(/^file:\/\/\//, "")
            pathField.text = value.replace(/\//g, "\\")
        }
    }
}
