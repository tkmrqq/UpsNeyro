import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    required property UpscaleManager upscaleManager

    SystemMonitor {
        id: sysMon
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 25

            Label {
                text: "Resource Monitor"
                font.pixelSize: 22
                color: Theme.textPrimary
            }

            SettingsSection {
                title: "Processing Hardware"

                StatBar {
                    title: "GPU Load (" + sysMon.gpuName + ")"
                    value: sysMon.gpuLoad
                    barColor: Theme.accentSecondary
                }

                StatBar {
                    title: "GPU VRAM"
                    value: sysMon.vramLoad
                    textValue: sysMon.vramText
                    barColor: Theme.accentSecondary
                }
            }

            SettingsSection {
                title: "System Hardware"

                StatBar {
                    title: "CPU Load (" + sysMon.cpuName + ")"
                    value: sysMon.cpuLoad
                    barColor: Theme.accent
                }

                StatBar {
                    title: "System RAM"
                    value: sysMon.ramLoad
                    textValue: sysMon.ramText
                    barColor: Theme.accent
                }
            }

            // ── ETA блок ─────────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 10
                height: 80
                radius: 8
                color: "#2a2a32"

                ColumnLayout {
                    anchors.centerIn: parent

                    Label {
                        text: "Estimated Time Remaining"
                        color: Theme.textSecondary
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        // upscaleManager — твой синглтон/контекст объект в QML
                        text: upscaleManager && upscaleManager.upscaleEta !== "" ? upscaleManager.upscaleEta : "--:--"
                        color: Theme.textPrimary
                        font.pixelSize: 24
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // ── Pipeline stats блок ───────────────────────────────────────────
            SettingsSection {
                title: "Pipeline Performance"
                visible: upscaleManager ? upscaleManager.upscaleBusy : false

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    // FPS
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Frames/sec"
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Label {
                            text: upscaleManager.perfMonitor.currentFps.toFixed(2)
                            color: Theme.textPrimary
                            font.family: "monospace"
                        }
                    }

                    // Время апскейла
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Upscale"
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Label {
                            text: upscaleManager.perfMonitor.avgUpscaleMs.toFixed(1) + " ms"
                            color: upscaleManager.perfMonitor.bottleneck === "upscale"
                                   ? Theme.accentSecondary : Theme.textPrimary
                            font.family: "monospace"
                        }
                    }

                    // Время фильтров
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Filters"
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Label {
                            text: upscaleManager.perfMonitor.avgFilterMs.toFixed(1) + " ms"
                            color: upscaleManager.perfMonitor.bottleneck === "filter"
                                   ? Theme.accentSecondary : Theme.textPrimary
                            font.family: "monospace"
                        }
                    }

                    // Время энкодинга
                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Encode"
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Label {
                            text: upscaleManager.perfMonitor.avgEncodeMs.toFixed(1) + " ms"
                            color: upscaleManager.perfMonitor.bottleneck === "encode"
                                   ? Theme.accentSecondary : Theme.textPrimary
                            font.family: "monospace"
                        }
                    }

                    // Bottleneck строка
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4

                        Label {
                            text: "Bottleneck"
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                        Label {
                            text: upscaleManager.perfMonitor.bottleneck.toUpperCase()
                            color: Theme.accentSecondary
                            font.bold: true
                            font.family: "monospace"
                        }
                    }
                }
            }
        }
    }
}
