import QtQuick
import QtQuick.Controls
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    Column {
        anchors.centerIn: parent
        spacing: 10

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Video Preview"
            color: Theme.textSecondary
            font.pixelSize: 18
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Drop your video file here or click to browse"
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
