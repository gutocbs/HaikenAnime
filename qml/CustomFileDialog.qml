import QtQuick 2.2
import QtQuick.Dialogs 1.0

FileDialog {
    id: fileDialog
    title: "Please choose a file"
    folder: shortcuts.home
    selectFolder: true
    onAccepted: {
        var dirStr = ""+fileDialog.folder
        dirStr = dirStr.replace("file:///","")
        if(mainClass.fsetDirectory(dirStr))
            modeloLista.append({"name": dirStr, "colorCode": "black"})
    }
    onRejected: {
        console.log("Canceled")
//        Qt.quit()
    }
    Component.onCompleted: visible = true
}
