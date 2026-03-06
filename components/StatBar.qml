// StatBar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

ColumnLayout {
    property string title: "Stat"
    property real value: 0
    property string textValue: ""
    property color barColor: Theme.accent

    spacing: 8
    Layout.fillWidth: true

    // 1-й ЭТАЖ: Название железа
    Label {
        text: title
        color: Theme.textSecondary
        font.pixelSize: 14
        Layout.fillWidth: true
        wrapMode: Text.Wrap
    }

    // 2-й ЭТАЖ: Прогресс-бар и значения (на одной линии)
    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        // Кастомный прогресс-бар
        Rectangle {
            Layout.fillWidth: true
            height: 8
            radius: 4
            color: "#33333a"

            Rectangle {
                // Математическая защита: не даем полоске вылезти за пределы,
                // если value из C++ вдруг придет больше 100
                width: parent.width * (Math.min(Math.max(value, 0), 100) / 100)
                height: parent.height
                radius: 4
                color: barColor

                Behavior on width { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
            }
        }

        // Значение в процентах или гигабайтах (справа от бара)
        Label {
            text: textValue !== "" ? textValue : Math.round(value) + "%"
            color: Theme.textPrimary
            font.pixelSize: 14
            font.bold: true
            horizontalAlignment: Text.AlignRight
        }
    }
}
