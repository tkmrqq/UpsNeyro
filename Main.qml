import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtCore
import UpsNeyro2 1.0 as UpsNeyroModule
import "components"

ApplicationWindow {
    id: root
    width: 1400
    height: 900
    visible: true
    title: "AI Video Enhancer"

    signal startProcessing(string inputVideo, string outputFolder, string mode)

    Settings {
        id: appSettings
        category: "Theme"
        property string activePreset: "Orange"
    }

    Component.onCompleted: {
        UpsNeyroModule.Theme.setAccentPreset(appSettings.activePreset)
    }

    Material.theme: Material.Dark
    Material.accent: UpsNeyroModule.Theme.accent
    color: UpsNeyroModule.Theme.background

    Sidebar {
        id: sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        onTabChanged: stackLayout.currentIndex = index
    }

    Rectangle {
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: UpsNeyroModule.Theme.background

        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 500
                spacing: 15

                VideoPreview {
                    id: previewComponent
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                BottomControls {
                    targetPlayer: previewComponent.mediaPlayer
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                }
            }

            StackLayout {
                id: stackLayout
                currentIndex: 1 // По умолчанию открыт Upscale
                Layout.preferredWidth: 350
                Layout.maximumWidth: 350
                Layout.minimumWidth: 350
                Layout.fillHeight: true

                FiltersPage {}          // Индекс 0
                UpscalePage {}          // Индекс 1
                ResourceMonitorPage {}  // Индекс 2
                SettingsPage {}         // Индекс 3
            }

        }
    }
}
