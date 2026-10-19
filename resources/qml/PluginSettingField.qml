import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Renders one schema field generically. Plugin-specific behavior stays in C++:
// this file only presents the field and forwards edits to controller.SetValue.
ColumnLayout {
    id: fieldRoot

    property var field: ({})
    property var controller

    readonly property Theme theme: Theme {}

    spacing: 5
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: fieldRoot.field.label + (fieldRoot.field.required ? " *" : "")
            color: theme.ink
            font.pixelSize: 12
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }
        Switch {
            visible: fieldRoot.field.type === "boolean"
            checked: fieldRoot.field.type === "boolean" && fieldRoot.field.value === true
            onToggled: function() { fieldRoot.controller.SetValue(fieldRoot.field.key, checked) }
            Accessible.name: fieldRoot.field.label
        }
    }

    Label {
        visible: (fieldRoot.field.description || "").length > 0
        text: fieldRoot.field.description || ""
        color: theme.muted
        font.pixelSize: 11
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    AppTextField {
        visible: fieldRoot.field.type === "text"
        Layout.fillWidth: true
        text: fieldRoot.field.type === "text" ? (fieldRoot.field.value || "") : ""
        onEditingFinished: if (text !== fieldRoot.field.value) fieldRoot.controller.SetValue(fieldRoot.field.key, text)
        Accessible.name: fieldRoot.field.label
    }

    AppSpinBox {
        visible: fieldRoot.field.type === "number"
        from: fieldRoot.field.minimum || 0
        to: fieldRoot.field.maximum || 1000000
        value: fieldRoot.field.type === "number" ? Number(fieldRoot.field.value) : 0
        editable: true
        onValueModified: fieldRoot.controller.SetValue(fieldRoot.field.key, value)
        Accessible.name: fieldRoot.field.label
    }

    AppComboBox {
        visible: fieldRoot.field.type === "selection"
        Layout.fillWidth: true
        model: fieldRoot.field.options || []
        textRole: "label"
        valueRole: "value"
        currentIndex: Math.max(0, indexOfValue(fieldRoot.field.value))
        onActivated: fieldRoot.controller.SetValue(fieldRoot.field.key, currentValue)
        Accessible.name: fieldRoot.field.label
    }

    RowLayout {
        visible: fieldRoot.field.type === "secret"
        Layout.fillWidth: true
        spacing: 8

        // Secrets are write-only: the stored value is never shown, only whether one exists.
        AppTextField {
            id: secretInput
            Layout.fillWidth: true
            echoMode: TextInput.Password
            text: ""
            placeholderText: fieldRoot.field.hasSecret ? qsTr("Valor salvo (oculto)") : qsTr("Não configurado")
            onEditingFinished: if (text.length > 0) fieldRoot.controller.SetValue(fieldRoot.field.key, text)
            Accessible.name: fieldRoot.field.label
        }
        AppButton {
            visible: fieldRoot.field.hasSecret === true
            text: qsTr("Remover")
            variant: "flat"
            onClicked: fieldRoot.controller.ClearSecret(fieldRoot.field.key)
        }
    }
}
