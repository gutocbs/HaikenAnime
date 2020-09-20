import QtQuick 2.2
import QtQuick.Dialogs 1.0

FileDialog {
    id: fileDialog
    title: "Please choose a file"
    folder: shortcuts.home
    selectFolder: false
    onAccepted: {
        var dirStr = ""+fileDialog.fileUrl
        dirStr = dirStr.replace("file:///","")
        preferredTorrentPath = dirStr
        configTorrent.preferredTorrentPath = dirStr
    }
    onRejected: {
        console.log("Canceled")
//        Qt.quit()
    }
    Component.onCompleted: visible = true
}
