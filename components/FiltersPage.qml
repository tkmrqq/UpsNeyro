import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8 // Рекомендую добавить скругления для красоты

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: "Filters"
            font.pixelSize: 22
            color: Theme.textPrimary // ИСПРАВЛЕНО: было Theme.text
        }

        CheckBox { text: "Denoise video" }
        CheckBox { text: "Sharpen" }
        CheckBox { text: "Color correction" }

        Slider {
            Layout.fillWidth: true // Заставляем слайдер растянуться
            from: 0
            to: 100
            value: 50
        }

        Item { Layout.fillHeight: true } // ДОБАВЛЕНО: прижимает элементы наверх
    }
}
