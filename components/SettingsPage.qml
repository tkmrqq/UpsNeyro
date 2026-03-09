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
                    spacing: 15
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: "#4f7cff"
                        border.color: Theme.textPrimary
                        // Выделяем белой рамкой, если он сейчас активен
                        border.width: Qt.colorEqual(Theme.accent,"#4f7cff") ? 2 : 0

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                Theme.setAccentPreset("Blue")
                                appSettings.activePreset = "Blue"
                            }
                        }
                    }

                    // Пресет Красный
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: "#ff4f4f"
                        border.color: Theme.textPrimary
                        border.width: Qt.colorEqual(Theme.accent, "#ff4f4f") ? 2 : 0

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                Theme.setAccentPreset("Red")
                                appSettings.activePreset = "Red"
                            }
                        }
                    }

                    // Пресет Зеленый
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: "#10b981"
                        border.color: Theme.textPrimary
                        border.width: Qt.colorEqual(Theme.accent, "#10b981") ? 2 : 0

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                Theme.setAccentPreset("Green")
                                appSettings.activePreset = "Green"
                            }
                        }
                    }

                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: "#ffa14f"
                        border.color: Theme.textPrimary
                        border.width: Qt.colorEqual(Theme.accent, "#ffa14f") ? 2 : 0

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                Theme.setAccentPreset("Orange")
                                appSettings.activePreset = "Orange"
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: "Output Directory"

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
