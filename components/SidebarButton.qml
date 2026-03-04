import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Button {
    id: control
    property bool isActive: false // Свойство: нажата ли кнопка сейчас?

    Layout.alignment: Qt.AlignHCenter
    implicitWidth: 46
    implicitHeight: 46

    background: Rectangle {
        // Если активна — заливаем акцентным цветом, если навели мышку — слегка подсвечиваем серым
        color: control.isActive ? Theme.accent : (control.hovered ? "#33333a" : "transparent")
        radius: 12

        // Плавная анимация изменения цвета
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    contentItem: Text {
        text: control.text
        font.pixelSize: 22
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        // Неактивные иконки делаем слегка тусклыми для стиля
        opacity: control.isActive ? 1.0 : 0.6

        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
