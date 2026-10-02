import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    property string title: ""
    property string description: ""
    property string stateLabel: ""
    property color accentColor: "#315d91"

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.72, 360)
        spacing: 12

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 54
            Layout.preferredHeight: 54
            radius: 27
            color: "#e2ebf6"

            Label {
                anchors.centerIn: parent
                text: "◇"
                color: accentColor
                font.pixelSize: 26
            }
        }

        Label {
            Layout.fillWidth: true
            text: stateLabel
            color: accentColor
            font.pixelSize: 10
            font.weight: Font.DemiBold
            font.letterSpacing: 1.2
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            text: title
            color: "#172033"
            font.pixelSize: 20
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            text: description
            color: "#68758a"
            font.pixelSize: 13
            lineHeight: 1.25
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
