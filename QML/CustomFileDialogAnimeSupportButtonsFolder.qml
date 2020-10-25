import QtQuick 2.2
import QtQuick.Dialogs 1.0

FileDialog {
    id: fileDialog
    title: "Please choose a folder"
    folder: shortcuts.home
    selectFolder: true
    onAccepted: {
        var dirStr = ""+fileDialog.folder
        dirStr = dirStr.replace("file:///","")
        mainClass.fselecionaPastaespecificaAnime(dirStr);
    }
    onRejected: {
        console.log("Canceled")
    }
    Component.onCompleted: visible = true
}
