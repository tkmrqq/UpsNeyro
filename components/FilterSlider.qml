import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

ColumnLayout {
    property string title: "Parameter"
    property real from: 0
    property real to: 100
    property alias value: slider.value // Связываем внешнее значение со слайдером

    spacing: 2
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true

        Label {
            text: title
            color: Theme.textPrimary
            font.pixelSize: 14
            Layout.fillWidth: true // Прижимает название влево, а проценты вправо
        }

        Label {
            // Округляем значение и добавляем знак процента
            text: Math.round(slider.value) + "%"
            color: Theme.accent // Выделяем цифры фирменным цветом
            font.pixelSize: 14
            font.bold: true
        }
    }

    Slider {
        id: slider
        from: parent.from
        to: parent.to
        value: 50
        Layout.fillWidth: true
    }
}
