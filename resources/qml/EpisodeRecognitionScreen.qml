import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// History of automatic recognitions. Progress was already applied by the application; this
// panel only shows what happened and lets the user delete entries from the local history.
ColumnLayout {
    id: recognitionScreen

    property var controller: recognitionController

    readonly property Theme theme: Theme {}

    function statusLabel(key) {
        switch (key) {
        case "pending": return qsTr("Em observação")
        case "incomplete": return qsTr("Encerrado antes do tempo mínimo")
        case "marked-watched": return qsTr("Marcado como assistido")
        case "already-watched": return qsTr("Já estava assistido")
        case "uncertain": return qsTr("Reconhecimento incerto")
        case "ignored": return qsTr("Ignorado")
        case "corrected": return qsTr("Corrigido")
        case "removed": return qsTr("Removido da lista")
        case "progress-reduced": return qsTr("Progresso reduzido")
        }
        return key
    }

    // Manual edits (Home and seasonal catalog) are recorded with the "manual" source.
    function pluginLabel(pluginId) {
        return pluginId === "manual" ? qsTr("Manual") : pluginId
    }

    function syncErrorLabel(key) {
        switch (key) {
        case "retryable": return qsTr("falha temporária de comunicação; nova tentativa automática")
        case "unauthorized": return qsTr("conta do AniList não autorizada; reconecte a conta")
        case "rejected": return qsTr("o AniList recusou a alteração de progresso")
        case "persistence": return qsTr("falha ao registrar o envio localmente")
        case "unknown": return qsTr("falha desconhecida ao enviar o progresso")
        }
        // Records from older versions stored the sentence itself.
        return key
    }

    function syncLabel(state, error) {
        if (state === "pending") return qsTr("Sincronização com o AniList pendente")
        if (state === "failed") return qsTr("Falha ao sincronizar com o AniList: %1").arg(syncErrorLabel(error))
        if (state === "synced") return qsTr("Sincronizado com o AniList")
        return ""
    }

    spacing: 12
    Component.onCompleted: if (controller) controller.Reload()

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Histórico de atualizações")
            color: theme.ink
            font.pixelSize: 14
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        AppButton {
            text: qsTr("Atualizar")
            variant: "secondary"
            onClicked: recognitionScreen.controller.Reload()
        }
    }

    Label {
        visible: recognitionScreen.controller.errorMessage.length > 0
        text: recognitionScreen.controller.errorMessage
        color: theme.danger
        font.pixelSize: 11
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    Label {
        visible: historyList.count === 0
        text: qsTr("Nenhum episódio reconhecido ainda.")
        color: theme.muted
        font.pixelSize: 11
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    ListView {
        id: historyList
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(contentHeight, 520)
        clip: true
        spacing: 8
        model: recognitionScreen.controller.records
        delegate: Rectangle {
            id: row
            required property var recordId
            required property string title
            required property var season
            required property int episode
            required property string statusKey
            required property string pluginId
            required property string syncState
            required property string syncError
            required property string updatedAt
            required property bool correctable

            width: ListView.view.width
            implicitHeight: rowLayout.implicitHeight + 24
            color: theme.surfaceSoft
            radius: 6
            border.color: theme.line

            RowLayout {
                id: rowLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                        // A removal or a reduction concerns the whole entry, not an episode.
                        text: row.statusKey === "removed" || row.statusKey === "progress-reduced" ? row.title
                              : row.season !== undefined && row.season !== null
                              ? qsTr("%1 — T%2 E%3").arg(row.title).arg(row.season).arg(row.episode)
                              : qsTr("%1 — Episódio %2").arg(row.title).arg(row.episode)
                        color: theme.ink
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: recognitionScreen.statusLabel(row.statusKey) + " · "
                              + recognitionScreen.pluginLabel(row.pluginId) + " · " + row.updatedAt
                        color: theme.muted
                        font.pixelSize: 11
                    }
                    Label {
                        visible: text.length > 0
                        text: recognitionScreen.syncLabel(row.syncState, row.syncError)
                        color: row.syncState === "failed" ? theme.danger : theme.muted
                        font.pixelSize: 11
                    }
                }
                AppButton {
                    visible: row.correctable
                    text: qsTr("Excluir")
                    variant: "flat"
                    // Local history only: progress stays; an unsent AniList progress change is discarded.
                    onClicked: recognitionScreen.controller.Remove(row.recordId)
                }
            }
        }
    }

    AppButton {
        visible: recognitionScreen.controller.hasMore
        text: qsTr("Carregar mais")
        variant: "flat"
        onClicked: recognitionScreen.controller.LoadMore()
    }
}
