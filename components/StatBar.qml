import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

ColumnLayout {
    property string title: "Stat"
    property real value: 0      // От 0 до 100
    property string textValue: "" // Текст справа (например "4.2 GB / 8 GB")
    property color barColor: Theme.accent

    spacing: 5
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true
        Label {
            text: title
            color: Theme.textSecondary
            font.pixelSize: 14
            Layout.fillWidth: true
        }
        Label {
            text: textValue !== "" ? textValue : Math.round(value) + "%"
            color: Theme.textPrimary
            font.pixelSize: 14
            font.bold: true
        }
    }

    // Кастомный прогресс-бар
    Rectangle {
        Layout.fillWidth: true
        height: 8
        radius: 4
        color: "#33333a" // Фон полоски

        Rectangle {
            width: parent.width * (value / 100)
            height: parent.height
            radius: 4
            color: barColor

            // Плавная анимация при изменении значений
            Behavior on width { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
        }
    }
}
