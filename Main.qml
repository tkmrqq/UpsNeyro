import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import UpsNeyro2 1.0 as UpsNeyroModule
import "components"

ApplicationWindow {
    id: root
    width: 1400
    height: 900
    visible: true
    title: "AI Video Enhancer"

    signal startProcessing(string inputVideo, string outputFolder, string mode)

    Material.theme: Material.Dark
    Material.accent: UpsNeyroModule.Theme.accent
    color: UpsNeyroModule.Theme.background

    Sidebar {
        id: sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        // width: 70 — лучше задать implicitWidth внутри самого Sidebar.qml
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
            anchors.margins: 20 // ДОБАВЛЕНО: отступы от краев экрана
            spacing: 20         // ИСПРАВЛЕНО: расстояние между плеером и правой панелью

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 500
                spacing: 15     // ДОБАВЛЕНО: расстояние между видео и нижним баром

                VideoPreview {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                BottomControls {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80 // ИСПРАВЛЕНО: подсказываем Layout'у нужную высоту
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
                ResourceMonitorPage {}  // Индекс 2 (ДОБАВЛЕНО)
                SettingsPage {}         // Индекс 3 (ДОБАВЛЕНО)
            }

        }
    }
}
