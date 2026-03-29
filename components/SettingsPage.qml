import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import UpsNeyro2 1.0

Popup {
    id: settingsPopup
    width: 650
    height: Math.min(800, Overlay.overlay.height - 100)
    // anchors.centerIn: parent
    anchors.centerIn: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.panel
        radius: 12
        border.color: Theme.border
        border.width: 1
    }

    Overlay.modal: Rectangle {
        // Заливаем весь остальной экран полупрозрачным черным.
        // #A6000000 = Черный цвет с ~65% непрозрачности (A6 в HEX)
        color: "#A6000000"

        // Плавная анимация появления затемнения
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    Button {
        id: closeBtn
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 15
        z: 10

        // 1. Жесткие размеры кнопки
        width: 24
        height: 24

        // 2. Убиваем ВСЕ скрытые отступы Material стиля!
        padding: 0
        topInset: 0
        bottomInset: 0
        leftInset: 0
        rightInset: 0

        // 3. Фон: прозрачный в покое, серый при наведении
        background: Rectangle {
            // Привязываемся строго к размерам кнопки
            anchors.fill: parent
            color: closeBtn.hovered ? "#33333a" : "transparent"
            radius: 6 // 16 для круга (32/2), 6 для квадрата со скруглениями
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        // 4. Текст: центрируем через родительский Item
        contentItem: Item {
            anchors.fill: parent // Занимаем все 32x32

            Text {
                anchors.centerIn: parent // Идеально по центру!
                text: "✖\uFE0E"
                color: Theme.textSecondary
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        onClicked: settingsPopup.close()
    }


    // color: Theme.panel
    //radius: 8

    ScrollView {
        anchors.fill: parent
        anchors.margins: 30
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

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
                            // cursorShape: Qt.PointingHandCursor
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
                            // cursorShape: Qt.PointingHandCursor
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
                            // cursorShape: Qt.PointingHandCursor
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
                            // cursorShape: Qt.PointingHandCursor
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
                        settingsManager.outputDir = selectedFolder.toString().replace(/^(file:\/{2,3})/, "")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    TextField {
                        id: outputDirField
                        Layout.fillWidth: true
                        text: settingsManager.outputDir

                        color: Theme.textPrimary
                        background: Rectangle { color: "#33333a"; radius: 6 }
                        padding: 10
                        readOnly: true
                    }

                    Button {
                        text: "Browse"
                        background: Rectangle { color: Theme.accent; radius: 6 }
                        contentItem: Text { text: parent.text; color: "white" }
                        onClicked: folderDialog.open()
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
