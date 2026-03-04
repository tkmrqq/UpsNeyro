import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

ColumnLayout {
    property string title: ""
    spacing: 10
    Layout.fillWidth: true

    Label {
        text: title
        font.pixelSize: 16
        color: Theme.textSecondary
    }
}
