import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

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
                    ResolutionButton { text: "1080p"; Layout.fillWidth: true }
                    ResolutionButton { text: "2K"; Layout.fillWidth: true }
                    ResolutionButton { text: "4K"; selected: true; Layout.fillWidth: true }
                    ResolutionButton { text: "8K"; Layout.fillWidth: true }
                }
            }

            SettingsSection {
                title: "AI Processing Mode"

                ColumnLayout {
                    RadioButton { text: "Fast" }
                    RadioButton { text: "Balanced"; checked: true }
                    RadioButton { text: "Quality" }
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
                           Layout.fillWidth: true // ДОБАВЛЕНО: растягиваем кнопку
                           Layout.topMargin: 10
            }
        }
    }
}
