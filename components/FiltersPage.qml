import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

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
                    from: -100
                    to: 100
                    value: 0
                }

                FilterSlider {
                    title: "Contrast"
                    from: -100
                    to: 100
                    value: 0
                }

                FilterSlider {
                    title: "Saturation"
                    from: 0
                    to: 200
                    value: 100
                }

                FilterSlider {
                    title: "Blur"
                    from: 0
                    to: 100
                    value: 0
                }
            }

            // --- БЛОК ПРЕСЕТОВ ---
            SettingsSection {
                title: "Preset Filters"
                Layout.fillWidth: true

                // Flow автоматически переносит элементы на новую строку, если они не помещаются
                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    // Переиспользуем вашу готовую кнопку из UpscalePage!
                    ResolutionButton { text: "Cinematic" }
                    ResolutionButton { text: "Vibrant" }
                    ResolutionButton { text: "B&W" }
                    ResolutionButton { text: "Vintage" }

                    // Кнопка сброса
                    ResolutionButton {
                        text: "Reset All"
                        // Можно добавить логику сброса по клику
                        // onClicked: { ... }
                    }
                }
            }

            // Распорка, чтобы элементы не разъезжались, если окно слишком высокое
            Item { Layout.fillHeight: true }
        }
    }
}
