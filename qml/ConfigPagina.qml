import QtQuick 2.12
import QtQuick.Controls 2.3
import Qt.labs.settings 1.0

Item {
    id: configuracoes
    width: 1920
    height: 814
    scale: 1

    Settings {
        id: settingsConfDiretorio
        category: "confDir"
    }
    Settings {
        id: settingsConfTorrent
        category: "confTorrent"
    }

    property var configDiretorio: {
      lowQuality: false
      detection: []
      players: []
    }

    property var configTorrent: {
        autodownload: false
        autodownloadTimer: []
        preferredTorrentPath: []
        downloadLists: []
        preferredSub: []
        preferredQuality: []
        generalFeed: []
        specificFeed: []
        saveFolder: []
    }


    Component.onCompleted: {
        //Diretorio
        if(settingsConfDiretorio.value("imagemQualityHigh", false) === "false")
            configDiretorio.lowQuality = false
        else
            configDiretorio.lowQuality = true
        configDiretorio.players = settingsConfDiretorio.value("players", "")
        configDiretorio.detection = settingsConfDiretorio.value("detection", "")

        //Torrent
        if(settingsConfTorrent.value("autodownload", false) === "false")
            configTorrent.autodownload = false
        else
            configTorrent.autodownload = true
        configTorrent.autodownloadTimer = settingsConfTorrent.value("autodownloadTimer", "10")
        configTorrent.downloadLists = settingsConfTorrent.value("downloadLists", "")
        configTorrent.preferredSub = settingsConfTorrent.value("preferredSub", "")
        configTorrent.preferredQuality = settingsConfTorrent.value("preferredQuality", "")
        configTorrent.generalFeed = settingsConfTorrent.value("generalFeed",
                                                              "https://www.tokyotosho.info/rss.php?filter=1,11&zwnj=0")
        configTorrent.specificFeed = settingsConfTorrent.value("specificFeed", "https://nyaa.si/?page=rss&c=1_2&f=0&q=%title%")
    }

    property var imagem: ""

    Row {
        id: row
        anchors.fill: parent

        Column {
            id: column
            width: 460
            height: 814
            leftPadding: 8

            ConfigOptions {
                id: configOptions
            }
        }
    }

    StackView {
        id: stackViewConfig
        x: column.width
        width: parent.width-column.width
        height: parent.height
    }


}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.5}D{i:2;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
