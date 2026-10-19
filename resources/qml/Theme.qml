import QtQuick 2.15

// Paleta única do app. Cada arquivo cria a sua instância (`readonly property Theme theme: Theme {}`) em vez
// de copiar as cores; um singleton não funcionaria nos testes, que carregam o QML direto da pasta.
QtObject {
    readonly property color ink: "#172033"
    readonly property color muted: "#68758a"
    readonly property color line: "#d7dee8"
    readonly property color surface: "#ffffff"
    readonly property color surfaceSoft: "#f6f8fb"
    readonly property color accent: "#315d91"
    readonly property color accentSoft: "#e2ebf6"
    readonly property color danger: "#b42318"
}
