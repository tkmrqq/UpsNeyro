// main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtCore
import UpsNeyro2 1.0
import "components"

ApplicationWindow {
    id: root
    width: 1400
    height: 900
    visible: true
    title: "AI Video Enhancer"

    signal startProcessing(string inputVideo, string outputFolder, string mode)

    UpscaleManager { id: upscaleManager }

    SettingsManager { id: settingsManager }

    RecentFilesModel { id: recentFiles }

    JobQueue {
        id: jobQueue
        upscaleManager:  upscaleManager
        settingsManager: settingsManager
    }

    Settings {
        id: appSettings
        category: "Theme"
        property string activePreset: "Orange"
        // property string outputDirectory: ""
    }

    Component.onCompleted: {
        Theme.setAccentPreset(appSettings.activePreset)
        Logger.info("App started, theme: " + appSettings.activePreset)
    }

    Material.theme: Material.Dark
    Material.accent: Theme.accent
    color: Theme.background

    // --- ЛЕВОЕ МЕНЮ ---
    Sidebar {
        id: sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        onTabChanged: (index) => {
            if (index === -1) {
                // Если пришел -1, значит пользователь кликнул по УЖЕ активной вкладке.
                // Переключаем видимость (открываем, если закрыто; закрываем, если открыто).
                rightPanel.targetWidth = (rightPanel.targetWidth === 0) ? 350 : 0
            } else {
                // Если пришел обычный индекс, меняем страницу и ГАРАНТИРУЕМ, что панель открыта.
                stackLayout.currentIndex = index
                rightPanel.targetWidth = 350
            }
        }
        onOpenSettings: settingsDialog.open()
    }

    // --- ОСНОВНАЯ ЗОНА (ВИДЕО + ПРАВАЯ ПАНЕЛЬ) ---
    Rectangle {
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: Theme.background

        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            // 1. ВИДЕОПЛЕЕР (Занимает всё оставшееся место)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 15

                VideoPreview {
                    id: previewComponent
                    recentFiles: recentFiles
                    onVideoLoaded: (path) => {
                        recentFiles.addFile(path)
                        Logger.info("Video loaded: " + path)
                    }
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                BottomControls {
                    targetPlayer: previewComponent.mediaPlayer
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                }
            }

            // 2. ПРАВАЯ ВЫДВИЖНАЯ ПАНЕЛЬ
            Rectangle {
                id: rightPanel

                property int targetWidth: 350

                // Настройки для RowLayout
                Layout.preferredWidth: width
                Layout.fillHeight: true

                width: targetWidth
                clip: true // Обрезает контент при сужении
                color: "transparent"

                Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

                // Внутренний контейнер, который всегда имеет ширину 350px.
                // Так как anchors использовать нельзя, мы просто задаем жесткий width и height.
                Item {
                    width: 350
                    height: rightPanel.height // Привязываем высоту к родителю

                    StackLayout {
                        id: stackLayout
                        anchors.fill: parent
                        currentIndex: 1

                        FiltersPage {
                            upscaleManager: upscaleManager
                            videoPath: previewComponent.selectedVideoPath
                            videoPositionMs: previewComponent.mediaPlayer.position
                        }          // Индекс 0
                        UpscalePage {
                            upscaleManager: upscaleManager
                        }          // Индекс 1
                        ResourceMonitorPage {
                            upscaleManager: upscaleManager
                        }          // Индекс 2
                        JobQueuePage {
                            jobQueue: jobQueue
                        }          // Индекс 3
                    }

                    Button {
                        anchors.top: parent.top
                        anchors.right: parent.right // Прилипает к правому краю этого Item (350px)
                        anchors.margins: 10
                        width: 32; height: 32
                        z: 10

                        // padding: 0
                        // topInset: 0
                        // bottomInset: 0
                        // leftInset: 0
                        // rightInset: 0

                        background: Rectangle {
                            color: parent.hovered ? "#33333a" : "transparent"
                            radius: 16
                        }
                        contentItem: Item {
                            anchors.fill: parent
                            Text {
                                anchors.fill: parent
                                text: "▶\uFE0E"
                                color: Theme.textSecondary
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        onClicked: rightPanel.targetWidth = 0
                    }
                }
            }
        }
    }

    Toast { id: globalToast }

    function showToast(msg, type = 0) {
        globalToast.message = msg
        globalToast.type = type
        globalToast.open()
    }

    SettingsPage {
        id: settingsDialog
    }
}
