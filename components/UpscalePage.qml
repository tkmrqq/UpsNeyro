import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    id: root
    color: Theme.panel
    radius: 8

    required property UpscaleManager upscaleManager
    required property Item videoPreview
    required property SettingsManager settingsManager
    required property JobQueue jobQueue

    signal toastRequested(string msg, int type)

    PreviewPopup { id: previewPopup }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 18

            // ── Заголовок ─────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: qsTr("Upscale")
                    font.pixelSize: 22
                    color: Theme.textPrimary
                }
                Label {
                    text: qsTr("Start or preview, then adjust size, quality, and model.")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    lineHeight: 1.25
                }
            }

            // ── Карточка: запуск ──────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: runCard.implicitHeight + 28
                radius: 10
                color: "#222228"
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: runCard
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 12

                    Label {
                        text: qsTr("Run")
                        font.pixelSize: 13
                        font.bold: true
                        color: Theme.textSecondary
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        PrimaryButton {
                            text: upscaleManager.upscaleBusy ? qsTr("Cancel") : qsTr("Start upscaling")
                            Layout.fillWidth: true
                            height: 44

                            background: Rectangle {
                                radius: 10
                                gradient: Gradient {
                                    GradientStop {
                                        position: 0
                                        color: upscaleManager.upscaleBusy
                                               ? "#c0392b" : Theme.accentGradientStart
                                    }
                                    GradientStop {
                                        position: 1
                                        color: upscaleManager.upscaleBusy
                                               ? "#922b21" : Theme.accentGradientEnd
                                    }
                                }
                            }

                            onClicked: {
                                if (upscaleManager.upscaleBusy) {
                                    upscaleManager.cancelUpscaling()
                                    return
                                }
                                if (videoPreview.selectedVideoPath === "") {
                                    root.toastRequested("Please select a video file first!", 2)
                                    return
                                }
                                upscaleManager.startUpscaling(
                                    videoPreview.selectedVideoPath,
                                    settingsManager.outputDir
                                )
                            }
                        }

                        PrimaryButton {
                            text: upscaleManager.previewBusy ? qsTr("Preview…") : qsTr("Preview")
                            Layout.fillWidth: true
                            height: 44
                            enabled: !upscaleManager.previewBusy && !upscaleManager.upscaleBusy

                            onClicked: {
                                if (videoPreview.selectedVideoPath === "") {
                                    root.toastRequested("Please select a video file first!", 2)
                                    return
                                }
                                previewPopup.originalSource = ""
                                previewPopup.upscaledSource = ""
                                previewPopup.open()

                                var posSec = videoPreview.mediaPlayer.position / 1000.0
                                upscaleManager.startPreview(
                                    videoPreview.selectedVideoPath,
                                    posSec,
                                    settingsManager.hardwareDecodePreview
                                )
                            }
                        }
                    }

                    PrimaryButton {
                        text: qsTr("Add to queue")
                        Layout.fillWidth: true
                        height: 44
                        enabled: videoPreview.selectedVideoPath !== ""
                        onClicked: {
                            jobQueue.addJob(videoPreview.selectedVideoPath)
                            root.toastRequested("Added to queue", 0)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 72
                        radius: 8
                        color: "#2a2a32"
                        visible: upscaleManager.upscaleBusy

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    text: upscaleManager.upscaleStatus
                                    color: Theme.textPrimary
                                    font.pixelSize: 13
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: upscaleManager.upscaleProgress + "%"
                                    color: Theme.accent
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                                Label {
                                    text: upscaleManager.upscaleEta !== ""
                                          ? ("ETA " + upscaleManager.upscaleEta) : ""
                                    color: Theme.textSecondary
                                    font.pixelSize: 12
                                    visible: upscaleManager.upscaleEta !== ""
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 6
                                radius: 3
                                color: "#3a3a44"

                                Rectangle {
                                    width: parent.width * (upscaleManager.upscaleProgress / 100)
                                    height: parent.height
                                    radius: parent.radius
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop {
                                            position: 0.0
                                            color: Theme.accentGradientStart
                                        }
                                        GradientStop {
                                            position: 1.0
                                            color: Theme.accentGradientEnd
                                        }
                                    }
                                    Behavior on width {
                                        NumberAnimation {
                                            duration: 300
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Карточка: размер и качество файла ──────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: outputCard.implicitHeight + 28
                radius: 10
                color: "#222228"
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: outputCard
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 14

                    Label {
                        text: qsTr("Output size & quality")
                        font.pixelSize: 13
                        font.bold: true
                        color: Theme.textSecondary
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        spacing: 8
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("Target resolution")
                            font.pixelSize: 12
                            color: Theme.textSecondary
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            ResolutionButton {
                                text: "1080p"
                                Layout.fillWidth: true
                                selected: upscaleManager.resolution === "1080p"
                                onClicked: upscaleManager.resolution = "1080p"
                            }
                            ResolutionButton {
                                text: "2K"
                                Layout.fillWidth: true
                                selected: upscaleManager.resolution === "2K"
                                onClicked: upscaleManager.resolution = "2K"
                            }
                            ResolutionButton {
                                text: "4K"
                                Layout.fillWidth: true
                                selected: upscaleManager.resolution === "4K"
                                onClicked: upscaleManager.resolution = "4K"
                            }
                            ResolutionButton {
                                text: "8K"
                                Layout.fillWidth: true
                                selected: upscaleManager.resolution === "8K"
                                onClicked: upscaleManager.resolution = "8K"
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.border
                    }

                    ColumnLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: qsTr("Video quality (bitrate / CRF)")
                                color: Theme.textSecondary
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }
                            Label {
                                text: Math.round(qualitySlider.value) + "%"
                                color: Theme.accent
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }

                        Slider {
                            id: qualitySlider
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            stepSize: 1
                            value: upscaleManager.outputQuality
                            onMoved: upscaleManager.outputQuality = Math.round(value)
                        }

                        Label {
                            text: Math.round(qualitySlider.value) >= 80
                                  ? qsTr("High quality · larger file")
                                  : Math.round(qualitySlider.value) >= 50
                                    ? qsTr("Balanced")
                                    : qsTr("Smaller file · lower quality")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.border
                    }

                    CheckBox {
                        text: qsTr("Denoise video")
                        checked: upscaleManager.denoise
                        onClicked: upscaleManager.denoise = checked
                    }

                    Label {
                        text: qsTr("More tuning options will be added later.")
                        color: Theme.textSecondary
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // ── Карточка: модель ИИ ────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: aiCard.implicitHeight + 28
                radius: 10
                color: "#222228"
                border.width: 1
                border.color: Theme.border

                ColumnLayout {
                    id: aiCard
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 14
                    spacing: 10

                    Label {
                        text: qsTr("AI model")
                        font.pixelSize: 13
                        font.bold: true
                        color: Theme.textSecondary
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: [
                            {
                                mode: UpscaleManager.FastMode,
                                label: "Fast",
                                hint: "realesr-animevideov3 · animation"
                            },
                            {
                                mode: UpscaleManager.BalancedMode,
                                label: "Balanced",
                                hint: "realesrgan-x4plus · general"
                            },
                            {
                                mode: UpscaleManager.QualityMode,
                                label: "Quality",
                                hint: "realesrgan-x4plus-anime · max detail"
                            }
                        ]

                        delegate: RadioDelegate {
                            id: radioDelegate
                            Layout.fillWidth: true
                            implicitHeight: 52
                            checked: upscaleManager.mode === modelData.mode
                            onClicked: upscaleManager.mode = modelData.mode

                            background: Rectangle {
                                radius: 8
                                color: radioDelegate.checked
                                       ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
                                       : radioDelegate.hovered ? "#2f2f38" : "#2a2a32"
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                            }

                            indicator: Item {
                                x: 14
                                width: 18
                                height: 18
                                anchors.verticalCenter: parent.verticalCenter

                                Rectangle {
                                    anchors.fill: parent
                                    radius: width / 2
                                    border.width: 2
                                    border.color: radioDelegate.checked
                                                  ? Theme.accent : Theme.textSecondary
                                    color: "transparent"
                                    Behavior on border.color {
                                        ColorAnimation {
                                            duration: 150
                                        }
                                    }

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 9
                                        height: 9
                                        radius: width / 2
                                        color: Theme.accent
                                        visible: radioDelegate.checked
                                        scale: radioDelegate.checked ? 1.0 : 0.0
                                        Behavior on scale {
                                            NumberAnimation {
                                                duration: 150
                                                easing.type: Easing.OutBack
                                            }
                                        }
                                    }
                                }
                            }

                            contentItem: Item {
                                anchors.fill: parent

                                Column {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 42
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 2

                                    Label {
                                        text: modelData.label
                                        color: Theme.textPrimary
                                        font.pixelSize: 14
                                        font.bold: radioDelegate.checked
                                    }
                                    Label {
                                        text: modelData.hint
                                        color: Theme.textSecondary
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignLeft
                        Layout.topMargin: 4
                        height: 26
                        width: modelTag.implicitWidth + 20
                        radius: 13
                        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                        border.width: 1
                        border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.3)

                        Label {
                            id: modelTag
                            anchors.centerIn: parent
                            text: qsTr("Model: %1").arg(upscaleManager.modelName)
                            color: Theme.accent
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 8
            }

            Connections {
                target: upscaleManager

                function onPreviewBusyChanged() {
                    previewPopup.processingBusy = upscaleManager.previewBusy
                }
                function onPreviewStatusChanged() {
                    previewPopup.processingText = upscaleManager.previewStatus
                }
                function onPreviewProgressChanged() {
                    previewPopup.processingValue = upscaleManager.previewProgress
                }
                function onPreviewReady(originalUrl, upscaledUrl) {
                    previewPopup.originalSource = originalUrl
                    previewPopup.upscaledSource = upscaledUrl
                }
                function onPreviewFailed(error) {
                    previewPopup.close()
                    root.toastRequested("Preview failed: " + error, 2)
                }

                function onUpscaleFinished(outputPath) {
                    root.toastRequested("Done! Saved to: " + outputPath, 0)
                }
                function onUpscaleFailed(error) {
                    root.toastRequested("Upscale failed: " + error, 2)
                }
            }
        }
    }
}
