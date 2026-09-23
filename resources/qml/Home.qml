import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: home

    property var controller: homeController

    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"

    Rectangle {
        anchors.fill: parent
        color: "#eef2f7"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            spacing: 18

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
                    text: qsTr("Sua biblioteca")
                    color: ink
                    font.pixelSize: 25
                    font.weight: Font.Bold
                }
            }

            Label {
                text: qsTr("BIBLIOTECA")
                color: accent
                font.pixelSize: 12
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignVCenter
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 28
                color: line
            }

            Label {
                text: qsTr("Configurações")
                color: muted
                font.pixelSize: 13
                Layout.alignment: Qt.AlignVCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: line
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Rectangle {
                Layout.preferredWidth: 540
                Layout.fillHeight: true
                color: surface
                radius: 8
                border.color: line
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: qsTr("Minha lista")
                                color: ink
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                            }

                            Label {
                                text: qsTr("%1 títulos salvos").arg(controller.mediaCount)
                                color: muted
                                font.pixelSize: 12
                            }
                        }

                        Label {
                            text: qsTr("TODOS")
                            color: accent
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: line
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 12

                        Repeater {
                            model: controller.mediaModel
                            delegate: MediaCard {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 142
                                title: model.title
                                status: model.status === 1 ? qsTr("Em lançamento") : qsTr("Concluído")
                                progress: model.progress
                                score: model.score
                                muted: false
                            }
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
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 18

                    Label {
                        text: qsTr("DETALHES")
                        color: accent
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.5
                    }

                    StatePanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: qsTr("Nenhum título selecionado")
                        description: controller.state === "error"
                                     ? controller.errorMessage
                                     : qsTr("Os detalhes da mídia selecionada aparecerão aqui quando sua biblioteca tiver dados.")
                        stateLabel: controller.state === "loading"
                                    ? qsTr("CARREGANDO")
                                    : controller.state === "error"
                                      ? qsTr("ERRO")
                                      : controller.state === "ready"
                                        ? qsTr("DADOS DISPONÍVEIS")
                                        : qsTr("ESTADO VAZIO")
                        accentColor: accent
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: line
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            text: qsTr("Carregamento")
                            color: muted
                            font.pixelSize: 12
                        }

                        Label {
                            text: qsTr("Pronto para integração")
                            color: ink
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: qsTr("Sem conexão")
                            color: muted
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 74
            color: surfaceSoft
            radius: 8
            border.color: line
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 20

                Label {
                    text: qsTr("PRÓXIMO PASSO")
                    color: accent
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Conectar esta tela aos dados salvos no banco local através da camada de integração.")
                    color: muted
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }

                Label {
                    text: qsTr("V2 · somente leitura")
                    color: muted
                    font.pixelSize: 11
                }
            }
        }
    }

    Component.onCompleted: controller.reload()
}
