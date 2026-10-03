import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id: slotAnime
    width: 260
    height: 150
    visible: true

    property var titulo: "What is lorem ipsum?"
    property var episodios: "1/12"
    property var nota: "7/10"
    property var posicao: 0
    property var lista: "Watching"
    property var imagem: "qrc:/qtquickplugin/images/template_image.png"
    property color corLabel: "#b9c9fa"
    property color corEpisodio: "black"
    property var idAnimeSlot: ""
    property bool isVisible: false


    function titleMaxLength(title) {
        var newTitle = title.trim();

        if(title.length > 60){
            newTitle = title.split(" ");
            newTitle.pop();
            newTitle = newTitle.join(" ").trim();

            if(newTitle.length > 60)
                newTitle = titleMaxLength(newTitle);

            if(!newTitle.endsWith("..."))
                newTitle = newTitle.concat("...");
        }

        return newTitle;
    }

    Row {
        id: row
        anchors.fill: parent

        Column {
            id: column
            width: 110
            height: 150

            Item {
                id: element5
                width: 110
                height: 150

                Image {
                    id: image
                    x: 0
                    y: 0
                    width: 110
                    height: 150
                    source: imagem
                    //            fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    id: selecionaAnime
                    width: 110
                    height: 150
                    onPressed: mainClass.finfoAnimeSelecionado(posicao)
                }
            }


            //        Rectangle {
            //            id: rectangle4
            //            color: "#df4f4f"
            //            anchors.fill: parent
            //        }
        }

        Column {
            id: column1
            spacing: 1
            Row {
                id: row1
                width: 150
                height: 50

                Item {
                    id: element6
                    width: 150
                    height: 50

                    Button {
                        id: button12
                        visible: true
                        text: titleMaxLength(titulo)
                        anchors.fill: parent
                        flat: true
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            visible: true
                            color: "#000000"
                            text: button12.text
                            elide: Text.ElideRight
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font.weight: Font.Normal
                            rightPadding: 5
                            leftPadding: 5
                            minimumPointSize: 7
                            minimumPixelSize: 7
                            font.pointSize: 9
                            fontSizeMode: Text.Fit
                            font.bold: false
                            font.family: "Tahoma"
                        }
                        clip: false
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: corLabel
                            radius: 2
                            border.color: corLabel
                            border.width: 1
                            implicitWidth: 100
                            implicitHeight: 40
                        }
                        enabled: true
                        spacing: 0
                        display: AbstractButton.TextOnly
                        hoverEnabled: false
                        focusPolicy: Qt.NoFocus
                        font.bold: false
                    }
                }
            }



            Row {
                id: row2
                width: 150
                height: 25

                Item {
                    id: element7
                    width: 150
                    height: 25

//                    Button {
//                        id: button1
//                        visible: true
//                        text: episodios
//                        anchors.fill: parent
//                        display: AbstractButton.TextOnly
//                        background: Rectangle {
//                            opacity: enabled ? 1 : 0.3
//                            color: corLabel
//                            radius: 2
//                            border.color: corLabel
//                            border.width: 1
//                            implicitHeight: 40
//                            implicitWidth: 100
//                        }
//                        enabled: true
//                        flat: true
//                        contentItem: Text {
//                            opacity: enabled ? 1.0 : 0.3
//                            color: corEpisodio
//                            text: button1.text
//                            elide: Text.ElideRight
//                            horizontalAlignment: Text.AlignHCenter
//                            verticalAlignment: Text.AlignVCenter
//                            wrapMode: Text.WordWrap
//                            font: button1.font
//                        }
//                        spacing: 0
//                        hoverEnabled: false
//                        focusPolicy: Qt.NoFocus
//                        font.pointSize: 8
//                        clip: false
//                    }

                    Button {
                        id: button9
                        visible: true
                        text: episodios
                        anchors.fill: parent
                        flat: true
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            visible: true
                            color: corEpisodio
                            text: button9.text
                            elide: Text.ElideRight
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            minimumPointSize: 7
                            minimumPixelSize: 7
                            font.pointSize: 8
                            fontSizeMode: Text.Fit
                            font.bold: false
                            font.family: "Tahoma"
                        }
                        clip: false
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: corLabel
                            radius: 2
                            border.color: corLabel
                            border.width: 1
                            implicitWidth: 100
                            implicitHeight: 40
                        }
                        spacing: 0
                        enabled: true
                        focusPolicy: Qt.NoFocus
                        display: AbstractButton.TextOnly
                        hoverEnabled: false
                        font.bold: false
                    }
                }
            }

            Row {
                id: row3
                width: 150
                height: 25


                Item {
                    id: element8
                    width: 150
                    height: 25

                    Button {
                        id: button11
                        visible: true
                        text: nota
                        anchors.fill: parent
                        flat: true
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            visible: true
                            color: "#000000"
                            text: button11.text
                            elide: Text.ElideRight
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            minimumPixelSize: 7
                            minimumPointSize: 7
                            font.pointSize: 8
                            fontSizeMode: Text.Fit
                            font.bold: false
                            font.family: "Tahoma"
                        }
                        clip: false
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: corLabel
                            radius: 2
                            border.color: corLabel
                            border.width: 1
                            implicitWidth: 100
                            implicitHeight: 40
                        }
                        spacing: 0
                        enabled: true
                        focusPolicy: Qt.NoFocus
                        hoverEnabled: false
                        display: AbstractButton.TextOnly
                        font.bold: false
                    }
                }
            }

            Row {
                id: row4
                width: 150
                height: 25

                Item {
                    id: element9
                    width: 150
                    height: 25

                    Button {
                        id: button10
                        visible: true
                        text: lista
                        anchors.fill: parent
                        flat: true
                        contentItem: Text {
                            opacity: enabled ? 1.0 : 0.3
                            visible: true
                            color: "#000000"
                            text: button10.text
                            elide: Text.ElideRight
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            minimumPointSize: 7
                            minimumPixelSize: 7
                            font.pointSize: 8
                            fontSizeMode: Text.Fit
                            font.bold: false
                            font.family: "Tahoma"
                        }
                        clip: false
                        background: Rectangle {
                            opacity: enabled ? 1 : 0.3
                            color: corLabel
                            radius: 2
                            border.color: corLabel
                            border.width: 1
                            implicitWidth: 100
                            implicitHeight: 40
                        }
                        enabled: true
                        spacing: 0
                        display: AbstractButton.TextOnly
                        hoverEnabled: false
                        focusPolicy: Qt.NoFocus
                        font.bold: false
                    }
                }
            }

        }

    }

}



/*##^##
Designer {
    D{i:0;formeditorZoom:1.5}
}
##^##*/
