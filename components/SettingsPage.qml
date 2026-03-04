import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0
import QtQuick.Dialogs

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
                text: "Global Settings"
                font.pixelSize: 22
                color: Theme.textPrimary
            }

            SettingsSection {
                title: "AI Hardware Acceleration"
                Flow {
                    Layout.fillWidth: true
                    spacing: 10
                    ResolutionButton { text: "Auto"; selected: true }
                    ResolutionButton { text: "NVIDIA CUDA" }
                    ResolutionButton { text: "AMD ROCm" }
                    ResolutionButton { text: "CPU Only" }
                }
            }

            SettingsSection {
                title: "Theme"
                RowLayout {
                    spacing: 10
                    ResolutionButton { text: "Dark"; selected: true; Layout.fillWidth: true }
                    ResolutionButton { text: "Light"; Layout.fillWidth: true }
                }
            }

            SettingsSection {
                title: "Output Directory"

                // ДОБАВЛЕНО: Диалог выбора папки
                FolderDialog {
                    id: folderDialog
                    title: "Choose Output Directory"
                    onAccepted: {
                        outputDirField.text = selectedFolder.toString().replace(/^(file:\/{2,3})/, "")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    TextField {
                        id: outputDirField
                        Layout.fillWidth: true
                        text: "C:/Users/Admin/Videos/Upscaled" // Дефолтный путь
                        color: Theme.textPrimary
                        background: Rectangle { color: "#33333a"; radius: 6 }
                        padding: 10
                        readOnly: true // ДОБАВЛЕНО: запрещаем ручной ввод
                    }

                    Button {
                        text: "Browse"
                        background: Rectangle { color: Theme.accent; radius: 6 }
                        contentItem: Text { text: parent.text; color: "white" }
                        onClicked: folderDialog.open() // Открываем диалог по клику
                    }
                }
            }

            SettingsSection {
                title: "Advanced"
                CheckBox { text: "Open folder when finished"; checked: true }
                CheckBox { text: "Play sound on completion" }
                CheckBox { text: "Hardware decoding for preview"; checked: true }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
