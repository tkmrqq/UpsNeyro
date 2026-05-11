import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import UpsNeyro2 1.0

Popup {
    id: settingsPopup

    signal toastRequested(string msg, int type)

    required property SettingsManager settingsManager
    required property Settings appSettings
    required property HardwareProfiler hardwareProfiler
    required property UpdateChecker updateChecker
    required property ProjectManager projectManager
    required property UpscaleManager upscaleManager
    required property Item videoPreview

    function sessionLocalPath(url) {
        var s = url.toString()
        if (s.indexOf("file:") === 0) {
            s = s.replace(/^file:\/{2,3}/, "")
            if (Qt.platform.os === "windows" && s.length >= 3 && s.charAt(0) === "/"
                    && s.charAt(2) === ":")
                s = s.substring(1)
        }
        return s
    }

    width: 650
    height: Math.min(800, Overlay.overlay.height - 100)
    // anchors.centerIn: parent
    anchors.centerIn: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: hardwareProfiler.refresh()

    Connections {
        target: updateChecker
        function onCheckFinished(updateAvailable, latestVersion, releaseNotes, downloadPage) {
            var hasUrl = settingsManager.updateManifestUrl
                    && settingsManager.updateManifestUrl.length > 0
            if (!hasUrl) {
                toastRequested(qsTr("Set a GitHub Releases API URL first."), 1)
                return
            }
            if (!updateAvailable && (!latestVersion || latestVersion.length === 0)) {
                toastRequested(qsTr("Update check failed (network or unexpected response)."), 1)
                return
            }
            if (updateAvailable)
                toastRequested(qsTr("Update available: %1").arg(latestVersion), 0)
            else
                toastRequested(qsTr("You are up to date (%1).").arg(latestVersion), 0)
        }
    }

    Connections {
        target: projectManager
        function onError(message) {
            toastRequested(message, 1)
        }
    }

    FileDialog {
        id: saveSessionDialog
        title: qsTr("Save session")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [ qsTr("Session JSON (*.json)") ]
        onAccepted: {
            var path = settingsPopup.sessionLocalPath(selectedFile)
            var map = {
                inputVideo: videoPreview.selectedVideoPath,
                outputDir: settingsManager.outputDir,
                resolution: upscaleManager.resolution,
                upscaleMode: upscaleManager.mode,
                denoise: upscaleManager.denoise,
                outputQuality: upscaleManager.outputQuality,
                themePreset: appSettings.activePreset
            }
            if (projectManager.saveSession(path, map))
                toastRequested(qsTr("Session saved."), 0)
        }
    }

    FileDialog {
        id: loadSessionDialog
        title: qsTr("Load session")
        fileMode: FileDialog.OpenFile
        nameFilters: [ qsTr("JSON (*.json)"), qsTr("All files (*)") ]
        onAccepted: {
            var path = settingsPopup.sessionLocalPath(selectedFile)
            var raw = projectManager.loadSession(path)
            if (Object.keys(raw).length === 0)
                return
            var data = projectManager.mergeWithDefaults(raw)
            if (data.outputDir !== undefined && data.outputDir !== null
                    && String(data.outputDir).length > 0)
                settingsManager.outputDir = data.outputDir
            if (data.resolution !== undefined)
                upscaleManager.resolution = data.resolution
            if (data.upscaleMode !== undefined)
                upscaleManager.mode = data.upscaleMode
            if (data.denoise !== undefined)
                upscaleManager.denoise = data.denoise
            if (data.outputQuality !== undefined)
                upscaleManager.outputQuality = data.outputQuality
            if (data.themePreset !== undefined && data.themePreset) {
                Theme.setAccentPreset(data.themePreset)
                appSettings.activePreset = data.themePreset
            }
            if (data.inputVideo !== undefined && data.inputVideo
                    && String(data.inputVideo).length > 0)
                videoPreview.loadLocalPath(String(data.inputVideo))
            toastRequested(qsTr("Session loaded."), 0)
        }
    }

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

        contentItem: Item {
            anchors.fill: parent

            TintedIcon {
                anchors.centerIn: parent
                size: 14
                iconSource: "qrc:/UpsNeyro2/icons/x.svg"
                tint: closeBtn.hovered ? Theme.textPrimary : Theme.textSecondary
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
                title: qsTr("AI hardware (detected)")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Runtime uses your build (CUDA when available, otherwise CPU). Below is a quick probe of this machine.")
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: hardwareProfiler.cpuSummary.length ? hardwareProfiler.cpuSummary : "—"
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: hardwareProfiler.gpuSummary.length ? hardwareProfiler.gpuSummary : "—"
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: hardwareProfiler.recommendedDevice.length
                                ? ("Recommended: " + hardwareProfiler.recommendedDevice)
                                : "—"
                        color: Theme.accent
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Label {
                        text: hardwareProfiler.recommendationHint.length
                                ? hardwareProfiler.recommendationHint
                                : ""
                        visible: hardwareProfiler.recommendationHint.length > 0
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Refresh detection")
                        flat: true
                        onClicked: hardwareProfiler.refresh()
                    }
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
                title: qsTr("Updates")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Current version: %1").arg(updateChecker.currentVersion)
                        color: Theme.textPrimary
                        font.pixelSize: 12
                    }
                    Label {
                        text: qsTr("GitHub Releases API URL (latest release endpoint).")
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "https://api.github.com/repos/OWNER/REPO/releases/latest"
                        text: settingsManager.updateManifestUrl
                        color: Theme.textPrimary
                        selectByMouse: true
                        onEditingFinished: {
                            if (text !== settingsManager.updateManifestUrl)
                                settingsManager.updateManifestUrl = text.trim()
                            updateChecker.manifestUrl = settingsManager.updateManifestUrl
                        }
                        background: Rectangle {
                            color: "#33333a"
                            radius: 6
                        }
                    }
                    Button {
                        text: qsTr("Check for updates")
                        background: Rectangle {
                            color: Theme.accent
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: updateChecker.checkAsync()
                    }
                }
            }

            SettingsSection {
                title: qsTr("Export statistics")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label {
                        text: qsTr("Total: %1").arg(settingsManager.exportTotal)
                        color: Theme.textPrimary
                        font.pixelSize: 12
                    }
                    Label {
                        text: qsTr("Succeeded: %1 · Failed: %2 · Cancelled: %3")
                              .arg(settingsManager.exportSucceeded)
                              .arg(settingsManager.exportFailed)
                              .arg(settingsManager.exportCancelled)
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                    Button {
                        text: qsTr("Reset counters")
                        flat: true
                        onClicked: settingsManager.resetExportStats()
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
                title: qsTr("Session")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Save or restore video path, output folder, upscale options, and theme.")
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Button {
                            text: qsTr("Save session…")
                            Layout.fillWidth: true
                            background: Rectangle {
                                color: "#33333a"
                                radius: 6
                            }
                            contentItem: Text {
                                text: parent.text
                                color: Theme.textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: saveSessionDialog.open()
                        }
                        Button {
                            text: qsTr("Load session…")
                            Layout.fillWidth: true
                            background: Rectangle {
                                color: "#33333a"
                                radius: 6
                            }
                            contentItem: Text {
                                text: parent.text
                                color: Theme.textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: loadSessionDialog.open()
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Advanced")
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Label {
                        text: qsTr("Behaviour after export and preview capture.")
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        text: qsTr("Open folder when finished")
                        checked: settingsManager.openFolderWhenFinished
                        onCheckedChanged: {
                            if (checked !== settingsManager.openFolderWhenFinished)
                                settingsManager.openFolderWhenFinished = checked
                        }
                    }
                    CheckBox {
                        text: qsTr("Play sound on completion")
                        checked: settingsManager.playSoundOnComplete
                        onCheckedChanged: {
                            if (checked !== settingsManager.playSoundOnComplete)
                                settingsManager.playSoundOnComplete = checked
                        }
                    }
                    CheckBox {
                        text: qsTr("Hardware decoding for preview (CUDA/D3D12/VAAPI when available)")
                        checked: settingsManager.hardwareDecodePreview
                        onCheckedChanged: {
                            if (checked !== settingsManager.hardwareDecodePreview)
                                settingsManager.hardwareDecodePreview = checked
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Label {
                            text: qsTr("Queue retries after failure")
                            color: Theme.textPrimary
                            font.pixelSize: 12
                        }
                        Slider {
                            id: retrySlider
                            Layout.fillWidth: true
                            from: 0
                            to: 5
                            stepSize: 1
                            snapMode: Slider.SnapAlways
                            value: settingsManager.queueMaxRetries
                            onMoved: settingsManager.queueMaxRetries = Math.round(value)
                        }
                        Label {
                            text: settingsManager.queueMaxRetries
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            Layout.preferredWidth: 24
                        }
                    }
                    CheckBox {
                        text: qsTr("Verbose log file (include DEBUG lines)")
                        checked: settingsManager.logVerbose
                        onCheckedChanged: {
                            if (checked !== settingsManager.logVerbose)
                                settingsManager.logVerbose = checked
                        }
                    }
                    Label {
                        text: qsTr("Log file: %1").arg(Logger.logFilePath())
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Show log in file manager")
                        flat: true
                        onClicked: settingsManager.openLogFileLocation()
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
