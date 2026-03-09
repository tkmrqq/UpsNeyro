pragma Singleton
import QtQuick
import QtCore

QtObject {
    id: root

    // Основные цвета
    readonly property color background: "#1c1c1f"
    readonly property color panel: "#25252b"
    readonly property color border: "#2f2f36"
    // Текст
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#aaaaaa"
    // Акцент
    property color accent: "#ffa14f"
    property color accentSecondary: "#ffcc8a"
    property color accentGradientStart: "#ffa14f"
    property color accentGradientEnd: "#ff914d"

    function setAccentPreset(presetName) {
        if (presetName === "Blue") {
            accent = "#4f7cff"
            accentSecondary = "#285efa"
            accentGradientStart = "#4f7cff"
            accentGradientEnd = "#a64dff"
        } else if (presetName === "Red") {
            accent = "#ff4f4f"
            accentSecondary = "#f72d2d"
            accentGradientStart = "#ff4f4f"
            accentGradientEnd = "#ff8c4f"
        } else if (presetName === "Green") {
            accent = "#10b981"
            accentSecondary = "#24e0a2"
            accentGradientStart = "#10b981"
            accentGradientEnd = "#3b82f6"
        } else if (presetName === "Orange") {
            accent = "#ffa14f"
            accentSecondary = "#ffcc8a"
            accentGradientStart = "#ffa14f"
            accentGradientEnd = "#ff914d"
        }
    }
}
