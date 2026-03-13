import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

ColumnLayout {
    property string title: "Parameter"
    property real from: 0
    property real to: 100
    property real value: 50

    signal userMovedSlider(real newValue)

    spacing: 2
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true

        Label {
            text: title
            color: Theme.textPrimary
            font.pixelSize: 14
            Layout.fillWidth: true
        }

        Label {
            text: Math.round(slider.value) + "%"
            color: Theme.accent
            font.pixelSize: 14
            font.bold: true
        }
    }

    Slider {
        id: slider
        from: parent.from
        to: parent.to
        Layout.fillWidth: true

        value: parent.value

        onMoved: {
            parent.userMovedSlider(slider.value)
        }
    }
}
