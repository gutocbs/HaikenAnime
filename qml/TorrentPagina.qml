import QtQuick 2.12
import QtQuick.Controls 2.3

Item {
    id: element
    width: 1920
    height: 814
    scale: 1

    ListView {
        width: 200
        height: 200
        ScrollBar.vertical: ScrollBar{
            visible: true
        }
        ListModel {
            id: modeloTorrent
        }
//        Component.onCompleted: {
//            if(modeloTorrent.count !== numeroAnosListas){
//                for(var i = 0; i < numeroAnosListas; i++){
//                    model.append({"ano": (2000+i).toString()})
//                }
//            }
//        }
        model: modeloTorrent
        delegate: TableDelegate{
            name: type
            subName: age
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.33000001311302185}D{i:2;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}
D{i:4;anchors_height:208;anchors_width:834;anchors_y:606}D{i:3;anchors_height:814;anchors_width:834;anchors_x:0;anchors_y:0}
D{i:1;anchors_height:400;anchors_width:200}
}
##^##*/
