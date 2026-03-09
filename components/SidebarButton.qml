// SidebarButton.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import UpsNeyro2 1.0

Button {
    id: control
    property bool isActive: false
    property string iconSource: "" // Путь к SVG

    Layout.alignment: Qt.AlignHCenter
    implicitWidth: 46
    implicitHeight: 46

    // Говорим кнопке показывать только иконку
    // display: AbstractButton.IconOnly

    background: Rectangle {
        color: control.isActive ? Theme.accent : (control.hovered ? "#33333a" : "transparent")
        radius: 12
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    //ICON
    contentItem: Item {
        anchors.fill: parent
        opacity: control.isActive ? 1.0 : 0.6
        Behavior on opacity { NumberAnimation { duration: 150 } }

        Image {
            id: svgIcon
            anchors.centerIn: parent
            source: control.iconSource
            sourceSize.width: 24
            sourceSize.height: 24
            fillMode: Image.PreserveAspectFit
            visible: false // Обязательно скрываем оригинал!
        }

        // Перекрашиватель
        MultiEffect {
            anchors.fill: svgIcon
            source: svgIcon
            brightness: 1.0
            colorization: 1.0 // 100% заливка цветом
            colorizationColor: "white" // В какой цвет красить
        }
    }
}
