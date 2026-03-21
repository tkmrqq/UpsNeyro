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

            // ── Кнопки Start / Preview ────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                PrimaryButton {
                    text: upscaleManager.upscaleBusy ? "Cancel" : "Start Upscaling"
                    Layout.fillWidth: true

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
                        if (previewComponent.selectedVideoPath === "") {
                            root.showToast("Please select a video file first!", 2)
                            return
                        }
                        upscaleManager.startUpscaling(
                            previewComponent.selectedVideoPath,
                            settingsManager.outputDir
                        )
                    }
                }

                PrimaryButton {
                    text: upscaleManager.previewBusy ? "Generating Preview..." : "Preview"
                    Layout.fillWidth: true
                    enabled: !upscaleManager.previewBusy && !upscaleManager.upscaleBusy

                    onClicked: {
                        if (previewComponent.selectedVideoPath === "") {
                            root.showToast("Please select a video file first!", 2)
                            return
                        }
                        previewPopup.originalSource = ""
                        previewPopup.upscaledSource = ""
                        previewPopup.open()

                        var posSec = previewComponent.mediaPlayer.position / 1000.0
                        upscaleManager.startPreview(
                            previewComponent.selectedVideoPath, posSec
                        )
                    }
                }
            }

            // ── Прогресс апскейла (виден только во время обработки) ───
            Rectangle {
                Layout.fillWidth: true
                height: 70
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
                                  ? "ETA " + upscaleManager.upscaleEta : ""
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
                                GradientStop { position: 0.0; color: Theme.accentGradientStart }
                                GradientStop { position: 1.0; color: Theme.accentGradientEnd }
                            }
                            Behavior on width { NumberAnimation { duration: 300 } }
                        }
                    }
                }
            }

            // ── AI Processing Mode ────────────────────────────────────
            SettingsSection {
                title: "AI Processing Mode"

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    Repeater {
                        model: [
                            { mode: UpscaleManager.FastMode,
                              label: "Fast",
                              hint: "realesr-animevideov3 · best for animation" },
                            { mode: UpscaleManager.BalancedMode,
                              label: "Balanced",
                              hint: "realesrgan-x4plus · universal" },
                            { mode: UpscaleManager.QualityMode,
                              label: "Quality",
                              hint: "realesrgan-x4plus-anime · max quality" }
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
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }

                            indicator: Item {
                                x: 14
                                width: 18; height: 18
                                anchors.verticalCenter: parent.verticalCenter

                                Rectangle {
                                    anchors.fill: parent
                                    radius: width / 2
                                    border.width: 2
                                    border.color: radioDelegate.checked
                                                  ? Theme.accent : Theme.textSecondary
                                    color: "transparent"
                                    Behavior on border.color { ColorAnimation { duration: 150 } }

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 9; height: 9
                                        radius: width / 2
                                        color: Theme.accent
                                        visible: radioDelegate.checked
                                        scale: radioDelegate.checked ? 1.0 : 0.0
                                        Behavior on scale {
                                            NumberAnimation { duration: 150; easing.type: Easing.OutBack }
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

                    // Тег текущей модели
                    Rectangle {
                        Layout.topMargin: 2
                        height: 24
                        width: modelTag.implicitWidth + 20
                        radius: 12
                        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)

                        Label {
                            id: modelTag
                            anchors.centerIn: parent
                            text: "Model: " + upscaleManager.modelName
                            color: Theme.accent
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }

            // ── Output Quality ────────────────────────────────────────
            SettingsSection {
                title: "Output Quality"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Video bitrate / CRF"
                            color: Theme.textSecondary
                            font.pixelSize: 13
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
                        from: 0; to: 100
                        stepSize: 1
                        value: upscaleManager.outputQuality
                        onMoved: upscaleManager.outputQuality = Math.round(value)
                    }

                    Label {
                        text: Math.round(qualitySlider.value) >= 80
                              ? "High quality · larger file"
                              : Math.round(qualitySlider.value) >= 50
                                ? "Balanced quality"
                                : "Small file · lower quality"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }
            }

            // ── Advanced Options ──────────────────────────────────────
            SettingsSection {
                title: "Advanced Options"
                ColumnLayout {
                    CheckBox {
                        text: "Denoise video"
                        checked: upscaleManager.denoise
                        onClicked: upscaleManager.denoise = checked
                    }
                    CheckBox { text: "Enhance details"; checked: true }
                    CheckBox { text: "Preserve film grain" }
                }
            }

            // ── Connections ───────────────────────────────────────────
            Connections {
                target: upscaleManager

                // Превью
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

                // Апскейл
                function onUpscaleFinished(outputPath) {
                    root.showToast("Done! Saved to: " + outputPath, 0)
                }
                function onUpscaleFailed(error) {
                    root.showToast("Upscale failed: " + error, 2)
                }
            }
        }
    }
}
