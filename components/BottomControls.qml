import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    implicitHeight: 80
    radius: 8
    color: Theme.panel

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Button { text: "⏮" }
        Button { text: "▶" }
        Button { text: "⏭" }

        Slider {
            Layout.fillWidth: true
        }

        Slider {
            Layout.preferredWidth: 120
        }
    }
}
