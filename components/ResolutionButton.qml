import QtQuick
import QtQuick.Controls
import UpsNeyro2 1.0

Button {
    id: control
    property bool selected: false

    background: Rectangle {
        radius: 6
        color: selected ? Theme.accent : "#33333a"
    }

    contentItem: Text {
        text: control.text
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
