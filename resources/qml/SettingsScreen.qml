pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts 1.15

Item {
    id: settingsScreen

    signal backRequested()
    property var controller: settingsController

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    property string activeSection: "overview"

    readonly property var sections: [
        { key: "overview", label: qsTr("Visão geral"), eyebrow: qsTr("INÍCIO"),
          title: qsTr("Configurações"), description: qsTr("Escolha uma categoria para ajustar como o Haiken Anime organiza, reconhece e apresenta sua biblioteca.") },
        { key: "account", label: qsTr("Conta e sincronização"), eyebrow: qsTr("CONTA"),
          title: qsTr("Conta e sincronização"), description: qsTr("Conecte seu serviço de listas e defina como notas e atualizações serão sincronizadas.") },
        { key: "library", label: qsTr("Biblioteca e reconhecimento"), eyebrow: qsTr("BIBLIOTECA"),
          title: qsTr("Biblioteca e reconhecimento"), description: qsTr("Defina pastas, reprodutores e regras usadas para reconhecer arquivos locais.") },
        { key: "filters", label: qsTr("Filtros e listas"), eyebrow: qsTr("ORGANIZAÇÃO"),
          title: qsTr("Filtros e listas"), description: qsTr("Escolha quais estados participam das visualizações e automações da biblioteca.") },
        { key: "downloads", label: qsTr("Downloads"), eyebrow: qsTr("DOWNLOADS"),
          title: qsTr("Downloads"), description: qsTr("Prepare preferências de torrent, qualidade, legendas e destino dos arquivos.") },
        { key: "appearance", label: qsTr("Aparência"), eyebrow: qsTr("INTERFACE"),
          title: qsTr("Aparência"), description: qsTr("Personalize a leitura da interface com opções de tema, fonte e capas.") },
        { key: "history", label: qsTr("Histórico"), eyebrow: qsTr("ATIVIDADE"),
          title: qsTr("Histórico"), description: qsTr("Consulte registros de reprodução, reconhecimento e alterações da biblioteca.") }
    ]

    function sectionData(key) {
        for (let section of sections) {
            if (section.key === key) return section
        }
        return sections[0]
    }

    function optionIndex(options, propertyName, value) {
        for (let index = 0; index < options.length; ++index) {
            if (options[index][propertyName] === value) return index
        }
        return -1
    }

    function scoreScaleIndex(options) {
        for (let index = 0; index < options.length; ++index) {
            if (options[index].minimum === controller.scoreMinimum
                    && options[index].maximum === controller.scoreMaximum
                    && options[index].step === controller.scoreStep) return index
        }
        return -1
    }

    function requestBack() {
        if (controller.dirty) discardDialog.open()
        else backRequested()
    }

    Rectangle { anchors.fill: parent; color: "#eef2f7" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            spacing: 14

            Button {
                text: qsTr("‹  Biblioteca")
                flat: true
                font.pixelSize: 13
                onClicked: settingsScreen.requestBack()
            }

            Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 28; color: line }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: qsTr("HAIKEN ANIME")
                    color: accent
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2.2
                }
                Label {
                    text: qsTr("Preferências")
                    color: ink
                    font.pixelSize: 25
                    font.weight: Font.Bold
                }
            }

            Label {
                text: controller.errorMessage.length > 0 ? controller.errorMessage
                      : controller.statusMessage.length > 0 ? controller.statusMessage
                      : controller.dirty ? qsTr("Alterações não salvas") : qsTr("Configurações atualizadas")
                color: controller.errorMessage.length > 0 ? "#b13b43" : muted
                font.pixelSize: 11
            }

            Button {
                text: qsTr("Salvar alterações")
                enabled: controller.dirty && controller.valid && !controller.saving
                onClicked: controller.Save()
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: line }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Rectangle {
                Layout.preferredWidth: 258
                Layout.fillHeight: true
                color: surface
                radius: 8
                border.color: line

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    Label {
                        Layout.leftMargin: 10
                        Layout.topMargin: 6
                        Layout.bottomMargin: 8
                        text: qsTr("CATEGORIAS")
                        color: muted
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.4
                    }

                    Repeater {
                        model: settingsScreen.sections
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool selected: settingsScreen.activeSection === modelData.key
                            Layout.fillWidth: true
                            Layout.preferredHeight: modelData.key === "overview" ? 42 : 50
                            radius: 6
                            color: selected ? accentSoft : navHover.hovered ? surfaceSoft : "transparent"
                            border.color: activeFocus ? accent : "transparent"
                            border.width: activeFocus ? 2 : 0
                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: modelData.label
                            Accessible.onPressAction: settingsScreen.activeSection = modelData.key

                            Rectangle {
                                visible: parent.selected
                                anchors.left: parent.left
                                anchors.leftMargin: 1
                                anchors.verticalCenter: parent.verticalCenter
                                width: 3
                                height: 24
                                radius: 2
                                color: accent
                            }

                            Label {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: parent.selected ? accent : ink
                                font.pixelSize: 12
                                font.weight: parent.selected ? Font.DemiBold : Font.Normal
                                wrapMode: Text.Wrap
                            }

                            HoverHandler { id: navHover }
                            TapHandler { onTapped: settingsScreen.activeSection = modelData.key }
                            Keys.onReturnPressed: settingsScreen.activeSection = modelData.key
                            Keys.onSpacePressed: settingsScreen.activeSection = modelData.key
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 78
                        color: surfaceSoft
                        radius: 6
                        border.color: line

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 11
                            spacing: 3
                            Label { text: qsTr("CONFIGURAÇÃO LOCAL"); color: accent; font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 1 }
                            Label { Layout.fillWidth: true; text: qsTr("As opções disponíveis nesta etapa são salvas localmente e aplicadas imediatamente."); color: muted; font.pixelSize: 10; wrapMode: Text.Wrap }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: surface
                radius: 8
                border.color: line
                clip: true

                ScrollView {
                    anchors.fill: parent
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width
                        spacing: 0

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 34
                            Layout.rightMargin: 34
                            Layout.topMargin: 30
                            Layout.bottomMargin: 24
                            spacing: 8

                            Label {
                                text: settingsScreen.sectionData(activeSection).eyebrow
                                color: accent
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.5
                            }
                            Label {
                                Layout.fillWidth: true
                                text: settingsScreen.sectionData(activeSection).title
                                color: ink
                                font.pixelSize: 24
                                font.weight: Font.Bold
                                wrapMode: Text.Wrap
                            }
                            Label {
                                Layout.fillWidth: true
                                text: settingsScreen.sectionData(activeSection).description
                                color: muted
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: line }

                        Loader {
                            Layout.fillWidth: true
                            Layout.leftMargin: 34
                            Layout.rightMargin: 34
                            Layout.topMargin: 26
                            Layout.bottomMargin: 34
                            sourceComponent: activeSection === "overview" ? overviewContent
                                           : activeSection === "account" ? accountContent
                                           : activeSection === "library" ? libraryContent
                                           : activeSection === "filters" ? filtersContent
                                           : activeSection === "downloads" ? downloadsContent
                                           : activeSection === "appearance" ? appearanceContent
                                           : historyContent
                        }
                    }
                }
            }
        }
    }

    component SectionCard: Rectangle {
        default property alias content: cardLayout.data
        Layout.fillWidth: true
        implicitHeight: cardLayout.implicitHeight + 34
        color: surfaceSoft
        radius: 7
        border.color: line
        ColumnLayout { id: cardLayout; anchors.fill: parent; anchors.margins: 17; spacing: 12 }
    }

    component FieldLabel: Label {
        color: muted
        font.pixelSize: 11
    }

    component SettingsComboBox: Basic.ComboBox {
        id: settingsCombo

        property string caption

        implicitHeight: 64
        leftPadding: 13
        rightPadding: 36

        background: Rectangle {
            radius: 5
            color: settingsCombo.enabled ? surface : surfaceSoft
            border.width: settingsCombo.activeFocus ? 2 : 1
            border.color: settingsCombo.activeFocus ? accent : line
        }

        contentItem: Item {
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: settingsCombo.caption
                    color: muted
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.8
                }

                Text {
                    width: parent.width
                    text: settingsCombo.displayText
                    color: settingsCombo.enabled ? ink : muted
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }
        }

        indicator: Text {
            x: settingsCombo.width - width - 13
            anchors.verticalCenter: parent.verticalCenter
            text: "⌄"
            color: settingsCombo.enabled ? accent : muted
            font.pixelSize: 15
        }

        delegate: Basic.ItemDelegate {
            required property int index
            required property var model

            width: settingsCombo.popup ? settingsCombo.popup.width : settingsCombo.width
            implicitHeight: 38
            highlighted: settingsCombo.highlightedIndex === index

            contentItem: Text {
                text: model[settingsCombo.textRole]
                color: ink
                font.pixelSize: 12
                font.weight: settingsCombo.currentIndex === index ? Font.DemiBold : Font.Normal
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: parent.highlighted ? "#eaf1f8"
                                          : settingsCombo.currentIndex === index ? "#f4f7fb" : surface
            }
        }
    }

    component StaticField: Rectangle {
        id: staticField
        property string label: ""
        property string value: ""
        Layout.fillWidth: true
        implicitHeight: 64
        color: surface
        radius: 5
        border.color: line
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 13
            anchors.rightMargin: 13
            anchors.topMargin: 8
            anchors.bottomMargin: 8
            spacing: 2
            FieldLabel { text: staticField.label.toUpperCase(); font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.8 }
            Label { Layout.fillWidth: true; text: staticField.value; color: ink; font.pixelSize: 12; font.weight: Font.DemiBold; elide: Text.ElideRight }
        }
    }

    component SettingSwitch: RowLayout {
        id: settingSwitch
        property string title: ""
        property string description: ""
        Layout.fillWidth: true
        spacing: 16
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label { Layout.fillWidth: true; text: settingSwitch.title; color: ink; font.pixelSize: 12; font.weight: Font.DemiBold; wrapMode: Text.Wrap }
            Label { Layout.fillWidth: true; text: settingSwitch.description; color: muted; font.pixelSize: 10; wrapMode: Text.Wrap }
        }
        Switch { checked: false }
    }

    Component {
        id: overviewContent
        ColumnLayout {
            spacing: 18
            Label { text: qsTr("O que você pode configurar"); color: ink; font.pixelSize: 16; font.weight: Font.DemiBold }
            GridLayout {
                Layout.fillWidth: true
                columns: width >= 760 ? 2 : 1
                columnSpacing: 14
                rowSpacing: 14
                Repeater {
                    model: settingsScreen.sections.slice(1)
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 112
                        color: surfaceSoft
                        radius: 7
                        border.color: overviewHover.hovered ? accent : line
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData.label
                        Accessible.description: modelData.description
                        Accessible.onPressAction: settingsScreen.activeSection = modelData.key
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 6
                            RowLayout {
                                Layout.fillWidth: true
                                Label { Layout.fillWidth: true; text: modelData.label; color: ink; font.pixelSize: 13; font.weight: Font.DemiBold }
                                Label { text: "›"; color: accent; font.pixelSize: 18 }
                            }
                            Label { Layout.fillWidth: true; Layout.fillHeight: true; text: modelData.description; color: muted; font.pixelSize: 10; wrapMode: Text.Wrap; verticalAlignment: Text.AlignTop }
                        }
                        HoverHandler { id: overviewHover }
                        TapHandler { onTapped: settingsScreen.activeSection = modelData.key }
                        Keys.onReturnPressed: settingsScreen.activeSection = modelData.key
                        Keys.onSpacePressed: settingsScreen.activeSection = modelData.key
                    }
                }
            }
        }
    }

    Component {
        id: accountContent
        ColumnLayout {
            spacing: 16
            SectionCard {
                Label { text: qsTr("Serviço conectado"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                GridLayout {
                    Layout.fillWidth: true; columns: 2; columnSpacing: 12; rowSpacing: 10
                    StaticField { label: qsTr("Serviço"); value: "AniList" }
                    StaticField { label: qsTr("Usuário"); value: qsTr("Nenhuma conta conectada") }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Button { text: qsTr("Conectar conta"); enabled: false }
                }
            }
            SectionCard {
                Label { text: qsTr("Preferências de sincronização"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Sincronização automática"); color: ink; font.pixelSize: 12; font.weight: Font.DemiBold }
                        Label { Layout.fillWidth: true; text: qsTr("Atualizar a biblioteca em segundo plano quando o aplicativo for iniciado."); color: muted; font.pixelSize: 10; wrapMode: Text.Wrap }
                    }
                    Switch {
                        checked: controller.synchronizationEnabled
                        onToggled: controller.SetSynchronizationEnabled(checked)
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: line }
                GridLayout { Layout.fillWidth: true; columns: 2; columnSpacing: 12; rowSpacing: 6
                    SettingsComboBox {
                        Layout.fillWidth: true
                        caption: qsTr("FORMATO DE NOTA")
                        model: controller.scoreScaleOptions
                        textRole: "label"
                        currentIndex: settingsScreen.scoreScaleIndex(model)
                        onActivated: {
                            const selected = model[index]
                            controller.SetScoreScale(selected.minimum, selected.maximum, selected.step)
                        }
                    }
                    SettingsComboBox {
                        Layout.fillWidth: true
                        enabled: controller.synchronizationEnabled
                        caption: qsTr("INTERVALO DE SINCRONIZAÇÃO")
                        model: controller.synchronizationIntervalOptions
                        textRole: "label"
                        currentIndex: settingsScreen.optionIndex(model, "value", controller.synchronizationIntervalMs)
                        onActivated: controller.SetSynchronizationInterval(model[index].value)
                    }
                }
            }
        }
    }

    Component {
        id: libraryContent
        ColumnLayout {
            spacing: 16
            SectionCard {
                Label { text: qsTr("Arquivos locais"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                StaticField { label: qsTr("Pasta principal da biblioteca"); value: qsTr("Nenhuma pasta selecionada") }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Button { text: qsTr("Selecionar pasta"); enabled: false }
                }
            }
            SectionCard {
                Label { text: qsTr("Reconhecimento"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                SettingSwitch { enabled: false; title: qsTr("Reconhecer arquivos automaticamente"); description: qsTr("Disponível em uma etapa futura.") }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: line }
                StaticField { label: qsTr("Reprodutor preferido"); value: qsTr("Padrão do Windows") }
            }
        }
    }

    Component {
        id: filtersContent
        ColumnLayout {
            spacing: 16
            SectionCard {
                Label { text: qsTr("Estados incluídos"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                Label { Layout.fillWidth: true; text: qsTr("Defina quais listas poderão participar de filtros e futuras automações."); color: muted; font.pixelSize: 10; wrapMode: Text.Wrap }
                Repeater {
                    model: [qsTr("Watching / Reading"), qsTr("Planning"), qsTr("On Hold"), qsTr("Dropped"), qsTr("Completed")]
                    delegate: SettingSwitch { required property string modelData; enabled: false; title: modelData; description: qsTr("Disponível em uma etapa futura.") }
                }
            }
        }
    }

    Component {
        id: downloadsContent
        ColumnLayout {
            spacing: 16
            SectionCard {
                Label { text: qsTr("Downloads automáticos"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                SettingSwitch { enabled: false; title: qsTr("Ativar downloads automáticos"); description: qsTr("Disponível em uma etapa futura.") }
                GridLayout { Layout.fillWidth: true; columns: 2; columnSpacing: 12
                    StaticField { label: qsTr("Qualidade preferida"); value: "1080p" }
                    StaticField { label: qsTr("Legenda preferida"); value: qsTr("Português") }
                }
                StaticField { label: qsTr("Pasta de destino"); value: qsTr("Nenhuma pasta selecionada") }
            }
            SectionCard {
                Label { text: qsTr("Fontes"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                StaticField { label: qsTr("Feed geral"); value: qsTr("Nenhuma fonte configurada") }
                StaticField { label: qsTr("Feed específico por título"); value: qsTr("Nenhuma fonte configurada") }
            }
        }
    }

    Component {
        id: appearanceContent
        ColumnLayout {
            spacing: 16
            SectionCard {
                Label { text: qsTr("Interface"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                GridLayout {
                    id: appearanceGrid
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 6

                    StaticField { Layout.preferredWidth: 1; label: qsTr("Tema"); value: qsTr("Claro") }
                    StaticField { Layout.preferredWidth: 1; label: qsTr("Fonte"); value: qsTr("Padrão do sistema") }
                    StaticField { Layout.preferredWidth: 1; label: qsTr("Tamanho do texto"); value: qsTr("Padrão") }

                    SettingsComboBox {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        caption: qsTr("QUALIDADE DAS CAPAS")
                        model: controller.coverQualityOptions
                        textRole: "label"
                        currentIndex: settingsScreen.optionIndex(model, "key", controller.coverQualityKey)
                        onActivated: controller.SetCoverQuality(model[index].key)
                    }
                }
            }
        }
    }

    Component {
        id: historyContent
        ColumnLayout {
            spacing: 16
            SectionCard {
                Label { text: qsTr("Histórico local"); color: ink; font.pixelSize: 14; font.weight: Font.DemiBold }
                Label { Layout.fillWidth: true; text: qsTr("Quando o histórico estiver conectado, atividades de reprodução, reconhecimento e sincronização aparecerão aqui."); color: muted; font.pixelSize: 11; wrapMode: Text.Wrap }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 120; color: surface; radius: 5; border.color: line
                    ColumnLayout { anchors.centerIn: parent; spacing: 5
                        Label { Layout.alignment: Qt.AlignHCenter; text: qsTr("Nenhuma atividade disponível"); color: ink; font.pixelSize: 12; font.weight: Font.DemiBold }
                        Label { Layout.alignment: Qt.AlignHCenter; text: qsTr("O histórico será preenchido em uma etapa posterior."); color: muted; font.pixelSize: 10 }
                    }
                }
            }
        }
    }

    Dialog {
        id: discardDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Descartar alterações?")
        standardButtons: Dialog.Discard | Dialog.Cancel
        onDiscarded: {
            controller.Discard()
            settingsScreen.backRequested()
        }
        Label {
            width: 360
            text: qsTr("Existem configurações alteradas que ainda não foram salvas.")
            color: ink
            wrapMode: Text.Wrap
        }
    }
}
