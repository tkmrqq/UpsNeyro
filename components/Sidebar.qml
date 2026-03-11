// Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    id: root
    signal tabChanged(int index)
    signal openSettings()

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
            iconSource: "qrc:/UpsNeyro2/icons/filters.svg"
            isActive: root.currentIndex === 0
            onClicked: {
                if (root.currentIndex === 0) {
                // Если уже открыта — переключаем состояние правой панели (сигнал)
                    root.tabChanged(-1) // Мы используем -1 как кодовый сигнал для "переключить видимость"
                } else {
                    root.currentIndex = 0
                    root.tabChanged(0)
                }
            }
        }

        // Вкладка 1: Upscale (по умолчанию)
        SidebarButton {
            iconSource: "qrc:/UpsNeyro2/icons/magic.svg"
            isActive: root.currentIndex === 1
            onClicked: {
                if (root.currentIndex === 1) {
                    root.tabChanged(-1)
                } else {
                    root.currentIndex = 1
                    root.tabChanged(1)
                }
            }
        }
        // Вкладка 2: Resource Monitor
        SidebarButton {
            iconSource: "qrc:/UpsNeyro2/icons/monitor.svg"
            isActive: root.currentIndex === 2
            onClicked: {
                if (root.currentIndex === 2) {
                    root.tabChanged(-1)
                } else {
                    root.currentIndex = 2
                    root.tabChanged(2)
                }
            }
        }

        // Распорка: выталкивает нижние элементы в самый низ панели
        Item { Layout.fillHeight: true }

        // Вкладка 3: Settings (Переместили на индекс 3)
        SidebarButton {
            iconSource: "qrc:/UpsNeyro2/icons/settings.svg"
            // isActive: root.currentIndex === 3
            onClicked: {
                // root.currentIndex = 3
                // root.tabChanged(3)
                root.openSettings()
            }
            Layout.bottomMargin: 10
        }
    }
}
