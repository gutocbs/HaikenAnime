import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    id: root

    visible: true
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 700
    title: qsTr("Haiken Anime")
    color: palette.window

    palette.window: "#eef2f7"
    palette.windowText: "#172033"
    palette.button: "#ffffff"
    palette.buttonText: "#172033"
    palette.highlight: "#315d91"

    Home {
        anchors.fill: parent
    }
}
