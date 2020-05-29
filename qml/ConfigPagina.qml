import QtQuick 2.12
import QtQuick.Controls 2.3

Item {
    id: element
    width: 1920
    height: 814
    scale: 1


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
