import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import UpsNeyro2 1.0

Button {
    height: 50

    background: Rectangle {
        radius: 10
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.accentGradientStart }
            GradientStop { position: 1; color: Theme.accentGradientEnd }
        }
    }

    contentItem: Text {
        text: control.text
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
