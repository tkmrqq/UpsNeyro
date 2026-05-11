// main.qml
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtCore
import UpsNeyro2 1.0
import "components"

ApplicationWindow {
    id: root
    width: uiSettings.windowWidth > 0 ? uiSettings.windowWidth : 1400
    height: uiSettings.windowHeight > 0 ? uiSettings.windowHeight : 900
    x: uiSettings.windowX >= 0 ? uiSettings.windowX : (Screen.width - width) / 2
    y: uiSettings.windowY >= 0 ? uiSettings.windowY : (Screen.height - height) / 2
    visible: true
    title: "AI Video Enhancer"

    signal startProcessing(string inputVideo, string outputFolder, string mode)

    Settings {
        id: uiSettings
        category: "UI"
        property int windowX: -1
        property int windowY: -1
        property int windowWidth: 0
        property int windowHeight: 0
        property int lastSidebarTab: 1
    }

    function applyExportFinishedActions(outputPath) {
        if (!outputPath || outputPath.length === 0)
            return
        if (settingsManager.openFolderWhenFinished)
            settingsManager.revealOutputInFileManager(outputPath)
        if (settingsManager.playSoundOnComplete)
            settingsManager.playCompletionSound()
    }

    UpscaleManager { id: upscaleManager }

    SettingsManager { id: settingsManager }

    HardwareProfiler { id: hardwareProfiler }
    UpdateChecker { id: updateChecker }
    ProjectManager { id: projectManager }

    Component.onCompleted: {
        Theme.setAccentPreset(appSettings.activePreset)
        Logger.info("App started, theme: " + appSettings.activePreset)
        if (settingsManager.updateManifestUrl.length > 0)
            updateChecker.manifestUrl = settingsManager.updateManifestUrl
        sidebar.currentIndex = uiSettings.lastSidebarTab
        stackLayout.currentIndex = uiSettings.lastSidebarTab
    }

    onClosing: function (close) {
        uiSettings.windowX = root.x
        uiSettings.windowY = root.y
        uiSettings.windowWidth = root.width
        uiSettings.windowHeight = root.height
        uiSettings.lastSidebarTab = sidebar.currentIndex
    }

    Connections {
        target: jobQueue
        function onJobFinished(index, outputPath) {
            applyExportFinishedActions(outputPath)
            settingsManager.recordExportResult(true, false)
        }
        function onJobFailed(index, error) {
            settingsManager.recordExportResult(false, error === "Cancelled")
        }
    }

    Connections {
        target: upscaleManager
        function onUpscaleFinished(outputPath) {
            applyExportFinishedActions(outputPath)
            if (!jobQueue.running)
                settingsManager.recordExportResult(true, false)
        }
        function onUpscaleFailed(error) {
            if (!jobQueue.running)
                settingsManager.recordExportResult(false, error === "Cancelled")
        }
    }

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
                uiSettings.lastSidebarTab = index
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
                            jobQueue: jobQueue
                            videoPath: previewComponent.selectedVideoPath
                            videoPositionMs: previewComponent.mediaPlayer.position
                            onToastRequested: (msg, type) => showToast(msg, type)
                        }          // Индекс 0
                        UpscalePage {
                            upscaleManager: upscaleManager
                            videoPreview: previewComponent
                            settingsManager: settingsManager
                            jobQueue: jobQueue
                            onToastRequested: (msg, type) => showToast(msg, type)
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
                        anchors.right: parent.right
                        anchors.margins: 10
                        width: 32
                        height: 32
                        z: 10
                        flat: true
                        padding: 0
                        topInset: 0
                        bottomInset: 0
                        leftInset: 0
                        rightInset: 0
                        hoverEnabled: true
                        Accessible.name: qsTr("Hide side panel")

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Hide panel")
                        ToolTip.delay: 500

                        background: Rectangle {
                            color: parent.hovered ? "#33333a" : "transparent"
                            radius: 16
                        }
                        contentItem: Item {
                            anchors.fill: parent
                            TintedIcon {
                                anchors.centerIn: parent
                                size: 20
                                iconSource: "qrc:/UpsNeyro2/icons/chevrons-left.svg"
                                tint: Theme.textSecondary
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
        settingsManager: settingsManager
        appSettings: appSettings
        hardwareProfiler: hardwareProfiler
        updateChecker: updateChecker
        projectManager: projectManager
        upscaleManager: upscaleManager
        videoPreview: previewComponent
        onToastRequested: (msg, type) => showToast(msg, type)
    }
}
