import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Dynamic settings for playback plugins (mpv, Plex). Fields come from each plugin's schema;
// actions appear only for the capabilities the selected plugin declares.
ColumnLayout {
    id: pluginScreen

    property var controller: pluginSettingsController

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color danger: "#b42318"
    readonly property var selectedPlugin: {
        const plugins = controller ? controller.plugins : []
        for (let plugin of plugins) {
            if (plugin.id === controller.selectedPluginId) return plugin
        }
        return ({})
    }

    spacing: 16

    RowLayout {
        spacing: 8
        Repeater {
            model: pluginScreen.controller ? pluginScreen.controller.plugins : []
            delegate: AppButton {
                required property var modelData
                text: modelData.displayName
                variant: modelData.id === pluginScreen.controller.selectedPluginId ? "primary" : "secondary"
                onClicked: pluginScreen.controller.SelectPlugin(modelData.id)
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: fieldsColumn.implicitHeight + 34
        color: pluginScreen.surfaceSoft
        radius: 7
        border.color: pluginScreen.line

        ColumnLayout {
            id: fieldsColumn
            anchors.fill: parent
            anchors.margins: 17
            spacing: 14

            Repeater {
                model: pluginScreen.controller ? pluginScreen.controller.fields : []
                delegate: PluginSettingField {
                    required property var modelData
                    field: modelData
                    controller: pluginScreen.controller
                }
            }
        }
    }

    Rectangle {
        visible: pluginScreen.selectedPlugin.browserAuthentication === true
        Layout.fillWidth: true
        implicitHeight: accountColumn.implicitHeight + 34
        color: pluginScreen.surfaceSoft
        radius: 7
        border.color: pluginScreen.line

        ColumnLayout {
            id: accountColumn
            anchors.fill: parent
            anchors.margins: 17
            spacing: 12

            Label {
                text: pluginScreen.controller.connectionState === "connected" ? qsTr("Conta conectada")
                    : pluginScreen.controller.connectionState === "connecting" ? qsTr("Aguardando login no navegador…")
                    : qsTr("Conta não conectada")
                color: pluginScreen.ink
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }
            RowLayout {
                spacing: 8
                AppButton {
                    visible: pluginScreen.controller.connectionState !== "connected"
                    enabled: pluginScreen.controller.connectionState !== "connecting"
                    text: qsTr("Conectar no navegador")
                    onClicked: pluginScreen.controller.Connect()
                }
                AppButton {
                    visible: pluginScreen.controller.connectionState === "connected"
                    text: qsTr("Desconectar")
                    variant: "danger"
                    onClicked: pluginScreen.controller.Disconnect()
                }
                AppButton {
                    visible: pluginScreen.controller.connectionState === "connected"
                    text: qsTr("Listar servidores")
                    variant: "secondary"
                    onClicked: pluginScreen.controller.LoadServers()
                }
            }

            Repeater {
                model: pluginScreen.controller.servers
                delegate: RadioButton {
                    required property var modelData
                    text: modelData.name
                    checked: modelData.selected === true
                    onClicked: pluginScreen.controller.SelectServer(modelData.id)
                }
            }

            AppButton {
                visible: pluginScreen.selectedPlugin.userSelection === true && pluginScreen.controller.servers.length > 0
                text: qsTr("Escolher usuários observados")
                variant: "secondary"
                onClicked: pluginScreen.controller.LoadUsers()
            }
            Repeater {
                model: pluginScreen.controller.users
                delegate: CheckBox {
                    required property var modelData
                    text: modelData.name
                    checked: modelData.selected === true
                    onToggled: function() { pluginScreen.controller.SetUserSelected(modelData.id, checked) }
                }
            }
        }
    }

    Label {
        visible: pluginScreen.controller.errorMessage.length > 0
        text: pluginScreen.controller.errorMessage
        color: pluginScreen.danger
        font.pixelSize: 11
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Label {
        visible: pluginScreen.controller.statusMessage.length > 0
        text: pluginScreen.controller.statusMessage
        color: pluginScreen.muted
        font.pixelSize: 11
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    RowLayout {
        spacing: 8
        AppButton {
            visible: pluginScreen.selectedPlugin.connectionTest === true
            enabled: !pluginScreen.controller.busy
            text: qsTr("Testar conexão")
            variant: "secondary"
            onClicked: pluginScreen.controller.TestConnection()
        }
        Item { Layout.fillWidth: true }
        AppButton {
            enabled: pluginScreen.controller.dirty
            text: qsTr("Descartar")
            variant: "flat"
            onClicked: pluginScreen.controller.Discard()
        }
        AppButton {
            enabled: pluginScreen.controller.dirty
            text: qsTr("Salvar")
            onClicked: pluginScreen.controller.Save()
        }
    }
}
