import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    id: root

    visible: true
    width: 1440
    height: 900
    minimumWidth: root.showingSettings ? 680 : 1100
    minimumHeight: 700
    title: qsTr("Haiken Anime")
    color: palette.window

    palette.window: "#eef2f7"
    palette.windowText: "#172033"
    palette.button: "#ffffff"
    palette.buttonText: "#172033"
    palette.highlight: "#315d91"

    property bool showingSettings: page === "settings"
    property string page: "home"

    Home {
        anchors.fill: parent
        visible: root.page === "home"
        onOpenSettingsRequested: root.page = "settings"
        onOpenSeasonalCatalogRequested: root.page = "seasonal"
    }

    SettingsScreen {
        anchors.fill: parent
        visible: root.page === "settings"
        onBackRequested: root.page = "home"
    }

    SeasonalCatalogScreen {
        anchors.fill: parent
        visible: root.page === "seasonal"
        onBackRequested: root.page = "home"
    }
}
