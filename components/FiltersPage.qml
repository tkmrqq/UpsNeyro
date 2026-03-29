import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    required property UpscaleManager upscaleManager
    property string videoPath: ""
    property real   videoPositionMs: 0
    property var fm: upscaleManager.filters

    PreviewPopup { id: filterPreviewPopup }

    FilterPreviewManager { id: filterPreview }

    PresetManager { id: presetManager }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 25

            Label {
                text: "Filters"
                font.pixelSize: 22
                color: Theme.textPrimary
            }

            // --- БЛОК ПОЛЗУНКОВ ---
            // Скрываем при kernel-пресетах — там слайдеры не применяются
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 15
                visible: !fm.hasKernelPreset
                opacity: visible ? 1.0 : 0.0

                Behavior on opacity { NumberAnimation { duration: 150 } }

                FilterSlider {
                    title: "Brightness"
                    from: -100; to: 100
                    value: fm.brightness
                    onUserMovedSlider: function(newValue) { fm.brightness = newValue }
                }
                FilterSlider {
                    title: "Contrast"
                    from: -100; to: 100
                    value: fm.contrast
                    onUserMovedSlider: function(newValue) { fm.contrast = newValue }
                }
                FilterSlider {
                    title: "Saturation"
                    from: 0; to: 200
                    value: fm.saturation
                    onUserMovedSlider: function(newValue) { fm.saturation = newValue }
                }
                FilterSlider {
                    title: "Hue"
                    from: -180; to: 180
                    value: fm.hue
                    onUserMovedSlider: function(newValue) { fm.hue = newValue }
                }
                FilterSlider {
                    title: "Sharpness"
                    from: 0; to: 100
                    value: fm.sharpness
                    onUserMovedSlider: function(newValue) { fm.sharpness = newValue }
                }
                FilterSlider {
                    title: "Blur"
                    from: 0; to: 100
                    value: fm.blur
                    onUserMovedSlider: function(newValue) { fm.blur = newValue }
                }
                FilterSlider {
                    title: "Vignette"
                    from: 0; to: 100
                    value: fm.vignette
                    onUserMovedSlider: function(newValue) { fm.vignette = newValue }
                }
                FilterSlider {
                    title: "Grain"
                    from: 0; to: 100
                    value: fm.grain
                    onUserMovedSlider: function(newValue) { fm.grain = newValue }
                }
            }

            Label {
                visible: fm.hasKernelPreset
                Layout.fillWidth: true
                text: "Слайдеры недоступны для этого пресета"
                color: Theme.textSecondary
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }

            // --- БЛОК ПРЕСЕТОВ ---
            SettingsSection {
                title: "Preset Filters"
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    // Строка 1: стилистические пресеты
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        ResolutionButton {
                            text: "Cinematic"
                            selected: fm.activePreset === FilterManager.PresetCinematic
                            onClicked: fm.applyPreset(FilterManager.PresetCinematic)
                        }
                        ResolutionButton {
                            text: "Vibrant"
                            selected: fm.activePreset === FilterManager.PresetVibrant
                            onClicked: fm.applyPreset(FilterManager.PresetVibrant)
                        }
                        ResolutionButton {
                            text: "B&W"
                            selected: fm.activePreset === FilterManager.PresetBW
                            onClicked: fm.applyPreset(FilterManager.PresetBW)
                        }
                        ResolutionButton {
                            text: "Vintage"
                            selected: fm.activePreset === FilterManager.PresetVintage
                            onClicked: fm.applyPreset(FilterManager.PresetVintage)
                        }
                    }

                    // Строка 2: kernel-пресеты (edge detection и т.д.)
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        ResolutionButton {
                            text: "Prewitt"
                            selected: fm.activePreset === FilterManager.PresetPrewitt
                            onClicked: fm.applyPreset(FilterManager.PresetPrewitt)
                        }
                        ResolutionButton {
                            text: "Emboss"
                            selected: fm.activePreset === FilterManager.PresetEmboss
                            onClicked: fm.applyPreset(FilterManager.PresetEmboss)
                        }
                        ResolutionButton {
                            text: "MinMax"
                            selected: fm.activePreset === FilterManager.PresetMinMax
                            onClicked: fm.applyPreset(FilterManager.PresetMinMax)
                        }
                    }

                    // Активный пресет + сброс
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: fm.activePreset !== FilterManager.PresetNone
                                  ? "Active: " + ["Cinematic","Vibrant","B&W","Vintage",
                                                   "Prewitt","Emboss","MinMax"][fm.activePreset]
                                  : "No preset active"
                            color: fm.activePreset !== FilterManager.PresetNone
                                   ? Theme.accent
                                   : Theme.textSecondary
                            font.pixelSize: 12
                        }

                        Item { Layout.fillWidth: true }

                        ResolutionButton {
                            text: "Reset All"
                            onClicked: fm.resetAll()
                        }
                    }
                }
            }

            SettingsSection {
                title: "My Presets"
                Layout.fillWidth: true
                visible: presetManager.presetNames.length > 3 // > 3 = есть пользовательские (первые 3 встроенные)

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            // Показываем только пользовательские (после встроенных)
                            model: presetManager.presetNames.length - 3

                            ResolutionButton {
                                text: presetManager.nameAt(index + 3)

                                onClicked: {
                                    presetManager.applyPresetTo(index + 3, fm)
                                }
                            }
                        }
                    }

                    // Кнопка сохранить текущие настройки как пресет
                    RowLayout {
                        Layout.fillWidth: true

                        TextField {
                            id: presetNameField
                            placeholderText: "Название пресета..."
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "Сохранить"
                            enabled: presetNameField.text.trim().length > 0
                            onClicked: {
                                presetManager.savePresetFromValues(
                                    presetNameField.text.trim(),
                                    fm.brightness, fm.contrast, fm.saturation, fm.hue,
                                    fm.sharpness, fm.blur, fm.vignette, fm.grain
                                )
                                presetNameField.text = ""
                            }
                        }
                    }
                }
            }

            Button {
                text: filterPreview.busy ? "Generating..." : "Filter Preview"
                enabled: videoPath !== "" && !filterPreview.busy
                onClicked: {
                    filterPreviewPopup.originalSource = ""
                    filterPreviewPopup.upscaledSource = ""
                    filterPreviewPopup.processingBusy = true
                    filterPreviewPopup.processingText = "Applying filters..."
                    filterPreviewPopup.open()
                    filterPreview.generate(videoPath, videoPositionMs / 1000.0, fm)
                }
            }

            Connections {
                target: filterPreview

                function onPreviewReady(originalUrl, filteredUrl) {
                    filterPreviewPopup.originalSource = originalUrl
                    filterPreviewPopup.upscaledSource = filteredUrl
                    filterPreviewPopup.processingBusy = false
                }

                function onPreviewFailed(error) {
                    filterPreviewPopup.close()
                    // показать toast через root если нужно
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
