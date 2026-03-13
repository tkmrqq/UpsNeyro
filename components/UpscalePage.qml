import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8
    UpscaleManager { id: upscaleManager }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20 // ДОБАВЛЕНО: внутренние отступы
        contentWidth: availableWidth
        clip: true // ДОБАВЛЕНО: чтобы контент не вылезал за скругления при скролле

        ColumnLayout {
            width: parent.width // Привязываем к ширине ScrollView
            spacing: 20

            Label {
                text: "Upscale Settings"
                font.pixelSize: 22
                color: Theme.textPrimary
            }

            SettingsSection {
                title: "Target Resolution"
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ResolutionButton {
                        text: "1080p";
                        Layout.fillWidth: true
                        selected: upscaleManager.resolution === "1080p"
                        onClicked: upscaleManager.resolution = "1080p"
                    }
                    ResolutionButton {
                        text: "2K";
                        Layout.fillWidth: true
                        selected: upscaleManager.resolution === "2K"
                        onClicked: upscaleManager.resolution = "2K"
                    }
                    ResolutionButton {
                        text: "4K";
                        Layout.fillWidth: true
                        selected: upscaleManager.resolution === "4K"
                        onClicked: upscaleManager.resolution = "4K"
                    }
                    ResolutionButton {
                        text: "8K";
                        Layout.fillWidth: true
                        selected: upscaleManager.resolution === "8K"
                        onClicked: upscaleManager.resolution = "8K"
                    }
                }
            }

            SettingsSection {
                title: "AI Processing Mode"

                ColumnLayout {
                    RadioButton {
                        text: "Fast"
                        checked: upscaleManager.mode === upscaleManager.FastMode
                        onClicked: upscaleManager.mode === upscaleManager.FastMode
                    }
                    RadioButton {
                        text: "Balanced";
                        checked: upscaleManager.mode === upscaleManager.BalancedMode
                        onClicked: upscaleManager.mode === upscaleManager.BalancedMode
                    }
                    RadioButton {
                        text: "Quality"
                        checked: upscaleManager.mode === upscaleManager.QualityMode
                        onClicked: upscaleManager.mode === upscaleManager.QualityMode
                    }
                }
            }

            SettingsSection {
                title: "Output Quality"

                Slider {
                    from: 0
                    to: 100
                    value: 80
                }
            }

            SettingsSection {
                title: "Advanced Options"

                ColumnLayout {
                    CheckBox { text: "Denoise video"; checked: true }
                    CheckBox { text: "Enhance details"; checked: true }
                    CheckBox { text: "Preserve film grain" }
                }
            }

            PrimaryButton {
                text: "Start Upscaling"
                Layout.fillWidth: true
                Layout.topMargin: 10

                onClicked: {
                    // Проверяем, выбрано ли видео (ищем его в VideoPreview)
                    // Для реального проекта VideoPreview лучше вынести в id: videoPreviewId
                    console.log("Button clicked! Sending signal to backend...")

                    // Вызываем сигнал из main.qml. Пока передаем тестовые данные.
                    // root.startProcessing("C:/test.mp4", "C:/output/", "4K")
                    upscaleManager.startUpscaling(previewComponent.selectedVideoPath, appSettings.outputDirectory)
                }
            }

        }
    }
}
