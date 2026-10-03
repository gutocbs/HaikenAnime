import QtQuick 2.0
import QtQuick.Controls 2.3


Item {
    property var name: "Nome Anime"
    property var subName: "Sub anime"
    property var resolution: "Resolution"
    property var episode: "Episode"
    property var fileName: "File Name"
    property var description: "Description"
    width: 700

    height: 50

    Row {
        id: row
        anchors.fill: parent

        Column {
            id: column
            width: 50
            height: parent.height

            Item {
                id: element
                width: parent.width
                height: parent.height

                CheckBox {
                    id: checkBox
                    display: AbstractButton.IconOnly
                    onCheckStateChanged: console.log(index + " - " + type + " = " + checked)
                }
            }
        }

        Column {
            id: column1
            width: 100
            height: parent.height
            Item {
                id: element1
                width: parent.width
                height: parent.height

                Label {
                    id: label
                    text: name
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                    anchors.fill: parent
                }
            }
        }

        Column {
            id: column2
            width: 100
            height: parent.height
            Item {
                id: element2
                width: parent.width
                height: parent.height
                Label {
                    id: label1
                    text: subName
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                }
            }
        }

        Column {
            id: column3
            width: 100
            height: parent.height
            Item {
                id: element3
                width: parent.width
                height: parent.height
                Label {
                    id: label2
                    text: resolution
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                }
            }
        }

        Column {
            id: column4
            width: 100
            height: parent.height
            Item {
                id: element4
                width: parent.width
                height: parent.height
                Label {
                    id: label3
                    text: episode
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                }
            }
        }

        Column {
            id: column5
            width: 100
            height: parent.height
            Item {
                id: element5
                width: parent.width
                height: parent.height
                Label {
                    id: label4
                    text: fileName
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                }
            }
        }

        Column {
            id: column6
            width: 100
            height: parent.height
            Item {
                id: element6
                width: parent.width
                height: parent.height
                Label {
                    id: label5
                    text: description
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Tahoma"
                }
            }
        }
    }


}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.6600000262260437}D{i:1}
}
##^##*/
