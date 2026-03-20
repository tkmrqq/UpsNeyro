import QtQuick
import QtQuick.Controls

Item {
    id: root
    property url originalSource: ""
    property url upscaledSource: ""
    property real splitPos: width / 2

    visible: originalSource.toString() !== "" && upscaledSource.toString() !== ""

    Image {
        anchors.fill: parent
        source: root.originalSource
        fillMode: Image.PreserveAspectFit
    }

    Item {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: root.splitPos
        clip: true

        Image {
            anchors.fill: parent
            source: root.upscaledSource
            fillMode: Image.PreserveAspectFit
        }
    }

    MouseArea {
        anchors.fill: parent
        onPositionChanged: function(mouse) {
            root.splitPos = Math.max(0, Math.min(mouse.x, root.width))
        }
    }

    Rectangle {
        x: root.splitPos - 1
        width: 2
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        color: "#ffffffaa"
    }
}
