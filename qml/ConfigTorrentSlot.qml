import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    width: 1020
    height: 814

    property var listaDownload: []
    property var autodownload: false
    property var autodownloadTimer: "10"
    property var preferredSub: ""
    property var preferredQuality: ""
    property var generalFeed: ""
    property var specificFeed: ""
    property var preferredTorrentPath: ""
    property var saveFolder: ""

    Component.onCompleted: {
        if(!configTorrent.autodownload)
            autoDownloadNo.checked = true
        else{
            autoDownloadYes.checked = true
            autodownload = true
        }
        autodownloadTimer = configTorrent.autodownloadTimer
        listaDownload = configTorrent.downloadLists.toString().split(",")
        preferredSub = configTorrent.preferredSub
        preferredQuality = configTorrent.preferredQuality
        generalFeed = configTorrent.generalFeed
        specificFeed = configTorrent.specificFeed
        preferredTorrentPath = configTorrent.preferredTorrentPath
        saveFolder = configTorrent.saveFolder
    }

    function fcheckState(lista){
        if(listaDownload.includes(lista))
            return true
        return false
    }

    Row {
        id: row14
        anchors.fill: parent


        Column {
            id: column
            width: 400
            height: parent.height

            Row {
                id: row1
                width: parent.width
                height: 50
                Item {
                    id: element
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label1
                        text: qsTr("Torrent Configuration")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: true
                        font.pointSize: 14
                    }
                }
            }

            Row {
                id: row2
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element1
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label2
                        text: qsTr("Preferred Torrent")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row3
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element2
                    width: parent.width
                    height: parent.height

                    TextField {
                        id: idPreferredTorrentPath
                        text: preferredTorrentPath
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        placeholderText: preferredTorrentPath
                    }
                }
            }
            Row {
                id: row24
                width: parent.width
                height: 50
                Item {
                    id: element21
                    width: parent.width
                    height: parent.height

                    Button {
                        id: button1
                        x: parent.width-width
                        text: qsTr("Select")
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: "Tahoma"
                        onClicked: stackViewTorrents.push("CustomFileDialogTorrent.qml")
                    }
                }
            }
            Row {
                id: row4
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element3
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label3
                        text: qsTr("Download torrents automatically")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }
            Row {
                id: row5
                x: 0
                width: parent.width
                height: 50

                Item {
                    id: element17
                    width: parent.width
                    height: parent.height

                    Frame {
                        id: frame
                        x: 0
                        y: 0
                        anchors.fill: parent

                        Row {
                            id: row
                            anchors.fill: parent

                            RadioButton {
                                id: autoDownloadYes
                                text: qsTr("Yes")
                                onClicked: configTorrent.autodownload = true
                            }

                            RadioButton {
                                id: autoDownloadNo
                                text: qsTr("No")
                                onClicked: configTorrent.autodownload = false
                            }
                        }
                    }
                }
            }
            Row {
                id: row6
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element5
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label5
                        text: qsTr("Download torrent after")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }
            Row {
                id: row7
                x: 0
                width: parent.width
                height: 50

                Column {
                    id: column1
                    width: 100
                    height: parent.height

                    Item {
                        id: element4
                        width: parent.width
                        height: parent.height

                        TextField {
                            id: textField
                            text: autodownloadTimer
                            onTextChanged: configTorrent.autodownloadTimer = text
                            horizontalAlignment: Text.AlignHCenter
                            placeholderText: "10"
                            anchors.fill: parent
                        }
                    }
                }

                Column {
                    id: column2
                    width: 100
                    height: parent.height
                    Item {
                        id: element6
                        width: parent.width
                        height: parent.height

                        Label {
                            id: label
                            text: qsTr("minutes")
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            anchors.fill: parent
                        }
                    }
                }
            }
            Row {
                id: row8
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element7
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label7
                        text: qsTr("Download animes from lists")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }
            Row {
                id: row9
                x: 0
                width: parent.width
                height: 100

                Item {
                    id: element8
                    width: parent.width
                    height: parent.height

                    Frame {
                        id: frame1
                        x: 0
                        y: 0
                        height: 50
                        anchors.fill: parent

                        Column {
                            id: column3
                            anchors.fill: parent

                            Row {
                                id: row17

                                CheckBox {
                                    id: checkBox
                                    width: 125
                                    text: qsTr("Watching")
                                    Component.onCompleted:{
                                        if(fcheckState("WATCHING"))
                                            checked = true
                                    }

                                    onCheckStateChanged: {
                                        if(listaDownload.includes("WATCHING") && checkState === 0)
                                            listaDownload.splice(listaDownload.indexOf("WATCHING"), 1)
                                        else if(checkState === 2)
                                            listaDownload.unshift("WATCHING")
                                        configTorrent.downloadLists = listaDownload
                                    }
                                }

                                CheckBox {
                                    id: checkBox1
                                    text: qsTr("Dropped")
                                    Component.onCompleted:{
                                        if(fcheckState("DROPPED"))
                                            checked = true
                                    }

                                    onCheckStateChanged: {
                                        if(listaDownload.includes("DROPPED") && checkState === 0)
                                            listaDownload.splice(listaDownload.indexOf("DROPPED"), 1)
                                        else if(checkState === 2)
                                            listaDownload.unshift("DROPPED")
                                        configTorrent.downloadLists = listaDownload
                                    }
                                }
                            }

                            Row {
                                id: row18
                                CheckBox {
                                    id: checkBox2
                                    text: qsTr("Plan to Watch")
                                    Component.onCompleted:{
                                        if(fcheckState("PLANNING"))
                                            checked = true
                                    }

                                    onCheckStateChanged: {
                                        if(listaDownload.includes("PLANNING") && checkState === 0)
                                            listaDownload.splice(listaDownload.indexOf("PLANNING"), 1)
                                        else if(checkState === 2)
                                            listaDownload.unshift("PLANNING")
                                        configTorrent.downloadLists = listaDownload
                                    }
                                }

                                CheckBox {
                                    id: checkBox3
                                    text: qsTr("On Hold")
                                    Component.onCompleted:{
                                        if(fcheckState("PAUSED"))
                                            checked = true
                                    }

                                    onCheckStateChanged: {
                                        if(listaDownload.includes("PAUSED") && checkState === 0)
                                            listaDownload.splice(listaDownload.indexOf("PAUSED"), 1)
                                        else if(checkState === 2)
                                            listaDownload.unshift("PAUSED")
                                        configTorrent.downloadLists = listaDownload
                                    }
                                }
                            }
                        }

                    }
                }
            }
            Row {
                id: row10
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element9
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label9
                        text: qsTr("Preferred sub")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }
            Row {
                id: row11
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element10
                    width: parent.width
                    height: parent.height

                    TextField {
                        id: idPreferredSub
                        height: parent.height
                        text: preferredSub
                        onTextChanged: configTorrent.preferredSub = text
                        placeholderText: "10"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
            Row {
                id: row12
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element11
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label11
                        text: qsTr("Prefered quality")
                        anchors.rightMargin: 0
                        anchors.bottomMargin: 0
                        anchors.leftMargin: 0
                        anchors.topMargin: 0
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }
            Row {
                id: row13
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element12
                    width: parent.width
                    height: parent.height

                    ComboBox {
                        id: quality
                        model: ["1080p", "720p", "480p"]
                        currentIndex: model.indexOf(preferredQuality)
                        onActivated: configTorrent.preferredQuality = quality.currentText
                    }
                }
            }
        }

        Column {
            id: column4
            width: 400
            height: parent.height

            Row {
                id: row15
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element13
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label10
                        text: qsTr("General anime torrent feed")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row16
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element14
                    width: parent.width
                    height: parent.height
                    TextField {
                        id: idgeneralFeed
                        text: generalFeed
                        anchors.fill: parent
                        placeholderText: "https://www.tokyotosho.info/rss.php?filter=1,11&zwnj=0"
                        onTextChanged: configTorrent.generalFeed = text
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Row {
                id: row19
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element15
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label12
                        text: qsTr("Specific anime torrent feed")
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row20
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element16
                    width: parent.width
                    height: parent.height
                    TextField {
                        id: idspecificFeed
                        text: specificFeed
                        anchors.fill: parent
                        placeholderText: "https://nyaa.si/?page=rss&c=1_2&f=0&q=%title%"
                        onTextChanged: configTorrent.specificFeed = text
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Row {
                id: row21
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element18
                    width: parent.width
                    height: parent.height
                    Label {
                        id: label4
                        text: qsTr("Save Folder")
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        font.family: "Tahoma"
                        font.bold: false
                        font.pointSize: 12
                    }
                }
            }

            Row {
                id: row22
                x: 0
                width: parent.width
                height: 50
                Item {
                    id: element19
                    width: parent.width
                    height: parent.height
                    TextField {
                        id: idSaveFolder
                        text: saveFolder
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        placeholderText: saveFolder
                    }
                }
            }

            Row {
                id: row25
                width: parent.width
                height: 50
                Item {
                    id: element22
                    width: parent.width
                    height: parent.height
                    Button {
                        id: button2
                        x: parent.width-width
                        text: qsTr("Select")
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: "Tahoma"
                        onClicked: stackViewTorrents.push("CustomFileDialogTorrentSaveFolder.qml")
                    }
                }
            }
        }
    }
    StackView {
        id: stackViewTorrents
    }

}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.75}D{i:17}D{i:16}D{i:35}D{i:34}D{i:2}D{i:60}D{i:66}D{i:1}
}
##^##*/
