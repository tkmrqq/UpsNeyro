import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    FilterManager {
        id: filterManager
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 25

            Label {
                text: "Filters"
                font.pixelSize: 22
                color: Theme.textPrimary
            }

            // --- БЛОК ПОЛЗУНКОВ ---
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 15

                FilterSlider {
                    title: "Brightness"
                    from: -100; to: 100
                    value: filterManager.brightness
                    onUserMovedSlider: function(newValue) {
                        filterManager.brightness = newValue
                    }
                }

                FilterSlider {
                    title: "Contrast"
                    from: -100; to: 100
                    value: filterManager.contrast
                    onUserMovedSlider: function(newValue) {
                        filterManager.contrast = newValue
                    }
                }

                FilterSlider {
                    title: "Saturation"
                    from: 0; to: 200
                    value: filterManager.saturation
                    onUserMovedSlider: function(newValue) {
                        filterManager.saturation = newValue
                    }
                }

                FilterSlider {
                    title: "Blur"
                    from: 0; to: 100
                    value: filterManager.blur
                    onUserMovedSlider: function(newValue) {
                        filterManager.blur = newValue
                    }
                }
            }

            // --- БЛОК ПРЕСЕТОВ ---
            SettingsSection {
                title: "Preset Filters"
                Layout.fillWidth: true

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    ResolutionButton {
                        text: "Cinematic"
                        selected: filterManager.activePreset === FilterManager.PresetCinematic
                        onClicked: filterManager.applyPreset(FilterManager.PresetCinematic)
                    }
                    ResolutionButton {
                        text: "Vibrant"
                        selected: filterManager.activePreset === FilterManager.PresetVibrant
                        onClicked: filterManager.applyPreset(FilterManager.PresetVibrant)
                    }
                    ResolutionButton {
                        text: "B&W"
                        selected: filterManager.activePreset === FilterManager.PresetBW
                        onClicked: filterManager.applyPreset(FilterManager.PresetBW)
                    }
                    ResolutionButton {
                        text: "Vintage"
                        selected: filterManager.activePreset === FilterManager.PresetVintage
                        onClicked: filterManager.applyPreset(FilterManager.PresetVintage)
                    }
                    ResolutionButton {
                        text: "Reset All"
                        onClicked: filterManager.resetAll()
                    }
                }
            }

            // Распорка, чтобы элементы не разъезжались, если окно слишком высокое
            Item { Layout.fillHeight: true }
        }
    }
}
