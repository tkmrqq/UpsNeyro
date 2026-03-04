pragma Singleton
import QtQuick

QtObject {

    // Основные цвета
    readonly property color background: "#1c1c1f"
    readonly property color panel: "#25252b"
    readonly property color border: "#2f2f36"

    // Текст
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#aaaaaa"

    // Акцент
    readonly property color accent: "#4f7cff"
    readonly property color accentGradientStart: "#4f7cff"
    readonly property color accentGradientEnd: "#a64dff"
}
