import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    UpscaleManager { id: upscaleManager }

    PreviewPopup { id: previewPopup }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 20

            Label {
                text: "Upscale Settings"
                font.pixelSize: 22
                color: Theme.textPrimary
            }

            // ── Target Resolution ─────────────────────────────────────
            SettingsSection {
                title: "Target Resolution"
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ResolutionButton {
                        text: "1080p"; Layout.fillWidth: true
                        selected: upscaleManager.resolution === "1080p"
                        onClicked: upscaleManager.resolution = "1080p"
                    }
                    ResolutionButton {
                        text: "2K"; Layout.fillWidth: true
                        selected: upscaleManager.resolution === "2K"
                        onClicked: upscaleManager.resolution = "2K"
                    }
                    ResolutionButton {
                        text: "4K"; Layout.fillWidth: true
                        selected: upscaleManager.resolution === "4K"
                        onClicked: upscaleManager.resolution = "4K"
                    }
                    ResolutionButton {
                        text: "8K"; Layout.fillWidth: true
                        selected: upscaleManager.resolution === "8K"
                        onClicked: upscaleManager.resolution = "8K"
                    }
                }
            }

            // ── AI Processing Mode ────────────────────────────────────
            SettingsSection {
                title: "AI Processing Mode"
                ColumnLayout {
                    RadioButton {
                        text: "Fast"
                        checked: upscaleManager.mode === UpscaleManager.FastMode
                        onClicked: upscaleManager.mode = UpscaleManager.FastMode
                    }
                    RadioButton {
                        text: "Balanced"
                        checked: upscaleManager.mode === UpscaleManager.BalancedMode
                        onClicked: upscaleManager.mode = UpscaleManager.BalancedMode
                    }
                    RadioButton {
                        text: "Quality"
                        checked: upscaleManager.mode === UpscaleManager.QualityMode
                        onClicked: upscaleManager.mode = UpscaleManager.QualityMode
                    }
                }
            }

            // ── Output Quality ────────────────────────────────────────
            SettingsSection {
                title: "Output Quality"
                Slider { from: 0; to: 100; value: 80 }
            }

            // ── Advanced Options ──────────────────────────────────────
            SettingsSection {
                title: "Advanced Options"
                ColumnLayout {
                    CheckBox { text: "Denoise video";        checked: true }
                    CheckBox { text: "Enhance details";      checked: true }
                    CheckBox { text: "Preserve film grain" }
                }
            }

            // ── Start Upscaling ───────────────────────────────────────
            PrimaryButton {
                text: "Start Upscaling"
                Layout.fillWidth: true
                Layout.topMargin: 10
                onClicked: {
                    if (previewComponent.selectedVideoPath === "") {
                        root.showToast("Please select a video file first!", 2)
                        return
                    }
                    root.showToast("Upscaling started...", 0)
                    upscaleManager.startUpscaling(
                        previewComponent.selectedVideoPath,
                        appSettings.outputDirectory
                    )
                }
            }

            // ── Preview ───────────────────────────────────────────────
            PrimaryButton {
                text: upscaleManager.previewBusy ? "Generating Preview..." : "Preview"
                Layout.fillWidth: true
                enabled: !upscaleManager.previewBusy

                onClicked: {
                    if (previewComponent.selectedVideoPath === "") {
                        root.showToast("Please select a video file first!", 2)
                        return
                    }
                    // Сбрасываем старые картинки и открываем попап со спиннером
                    previewPopup.originalSource  = ""
                    previewPopup.upscaledSource  = ""
                    previewPopup.open()

                    var posSec = previewComponent.mediaPlayer.position / 1000.0
                    upscaleManager.startPreview(
                        previewComponent.selectedVideoPath, posSec
                    )
                }
            }

            // ── Синхронизация состояния с попапом ────────────────────
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
                    root.showToast("Preview failed: " + error, 2)
                }
            }
        }
    }
}
