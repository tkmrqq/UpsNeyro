import QtQuick
import QtQuick.Effects

Item {
    id: root

    property url iconSource
    property color tint: "#ffffff"
    property int size: 20

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    Image {
        id: src
        anchors.centerIn: parent
        width: root.size
        height: root.size
        source: root.iconSource
        sourceSize.width: root.size
        sourceSize.height: root.size
        visible: false
    }

    MultiEffect {
        anchors.fill: src
        source: src
        brightness: 1.0
        colorization: 1.0
        colorizationColor: root.tint
    }
}
