import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    // Фейковые данные для анимации монитора
    property real cpuLoad: 15
    property real gpuLoad: 85
    property real ramLoad: 40
    property real vramLoad: 75

    Timer {
        interval: 1500; running: true; repeat: true
        onTriggered: {
            cpuLoad = Math.max(5, Math.min(100, cpuLoad + (Math.random() * 10 - 5)))
            gpuLoad = Math.max(50, Math.min(100, gpuLoad + (Math.random() * 20 - 10)))
        }
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
                    title: "GPU Load (NVIDIA RTX 4070)"
                    value: gpuLoad
                    barColor: Theme.accentSecondary
                }

                StatBar {
                    title: "GPU VRAM"
                    value: vramLoad
                    textValue: "9.2 GB / 12 GB"
                    barColor: Theme.accentSecondary
                }
            }

            SettingsSection {
                title: "System Hardware"

                StatBar {
                    title: "CPU Load (AMD Ryzen 7)"
                    value: cpuLoad
                    barColor: Theme.accent
                }

                StatBar {
                    title: "System RAM"
                    value: ramLoad
                    textValue: "16.4 GB / 32 GB"
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
