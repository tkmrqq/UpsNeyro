import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    SystemMonitor {
        id: sysMon
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true

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

            // Блок информации о текущей задаче
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 10
                height: 80
                radius: 8
                color: "#2a2a32"

                ColumnLayout {
                    anchors.centerIn: parent
                    Label { text: "Estimated Time Remaining"; color: Theme.textSecondary; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "00:14:32"; color: Theme.textPrimary; font.pixelSize: 24; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                }
            }
        }
    }
}
