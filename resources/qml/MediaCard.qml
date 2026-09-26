import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: card

    property string title: ""
    property string status: ""
    property string progress: ""
    property string score: ""
    property bool muted: false
    property bool selected: false
    property int mediaId: 0
    property url coverSource: "qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"
    signal coverLoadFailed(int mediaId)
    signal activated(int mediaId)

    color: selected ? "#edf4fc" : (muted ? "#f8fafc" : "#ffffff")
    radius: 6
    border.color: selected || activeFocus ? "#315d91" : "#e0e6ee"
    border.width: selected || activeFocus ? 2 : 1
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: title
    Accessible.onPressAction: card.activated(card.mediaId)

    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: card.activated(card.mediaId)
    }
    Keys.onReturnPressed: card.activated(card.mediaId)
    Keys.onSpacePressed: card.activated(card.mediaId)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 70
            Layout.fillHeight: true
            radius: 4
            color: muted ? "#e9eef5" : "#dbe7f4"

            Image {
                anchors.fill: parent
                source: card.coverSource
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
                sourceSize.width: 140
                sourceSize.height: 200
                onStatusChanged: if (status === Image.Error) card.coverLoadFailed(card.mediaId)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 7

            Label {
                Layout.fillWidth: true
                text: card.title
                color: muted ? "#8290a3" : "#172033"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Label {
                text: card.status
                color: "#8793a5"
                font.pixelSize: 11
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: card.progress
                    color: "#68758a"
                    font.pixelSize: 11
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#e4e9f0"
                }

                Label {
                    text: card.score
                    color: "#68758a"
                    font.pixelSize: 11
                }
            }
        }
    }
}
