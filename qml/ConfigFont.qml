import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Dialogs 1.3

Item {
    width: 1020
    height: 814

    Column {
        id: column
        anchors.fill: parent
//        FontDialog {
//            id: fontDialog
//            title: "Please choose a font"
//            font: Qt.font({ family: "Arial", pointSize: 24, weight: Font.Normal })
//            onAccepted: {
//                console.log("You chose: " + fontDialog.font)
//            }
//            onRejected: {
//                console.log("Canceled")
//            }
//            Component.onCompleted: visible = true
//        }
//        ColorDialog {
//            id: colorDialog
//            title: "Please choose a color"
//            onAccepted: {
//                console.log("You chose: " + colorDialog.color)
//            }
//            onRejected: {
//                console.log("Canceled")
//            }
//            Component.onCompleted: visible = true
//        }
    }
}
