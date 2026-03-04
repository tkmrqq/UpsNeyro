// Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    id: root
    signal tabChanged(int index)

    // ДОБАВЛЕНО: переменная, хранящая индекс открытой вкладки
    property int currentIndex: 1

    color: Theme.panel
    implicitWidth: 70

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        // Логотип приложения (сверху)
        Label {
            text: "🎬"
            font.pixelSize: 26
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            Layout.bottomMargin: 20
        }

        // Вкладка 0: Filters
        SidebarButton {
            text: "⚙️"
            isActive: root.currentIndex === 0
            onClicked: {
                root.currentIndex = 0
                root.tabChanged(0)
            }
        }

        // Вкладка 1: Upscale (по умолчанию)
        SidebarButton {
            text: "✨"
            isActive: root.currentIndex === 1
            onClicked: {
                root.currentIndex = 1
                root.tabChanged(1)
            }
        }

        // Распорка: выталкивает нижние элементы в самый низ панели
        Item { Layout.fillHeight: true }

        // Нижняя кнопка: общие настройки (пока без функционала, чисто для вида)
        SidebarButton {
            text: "🛠"
            isActive: root.currentIndex === 2
            onClicked: root.currentIndex = 2
            Layout.bottomMargin: 10
        }
    }
}
