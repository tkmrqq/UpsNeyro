import QtQuick
import QtQuick.Controls
import UpsNeyro2 1.0

Popup {
    id: root

    property string message: ""
    property int type: 0 // 0 = Info, 1 = Warning, 2 = Error

    // Popup не поддерживает anchors.horizontalCenter — центр через x
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: 80

    width: contentRow.width + 40
    height: 46

    // Не блокирует интерфейс!
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    // Анимация плавного появления сверху
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        NumberAnimation { property: "y"; from: 40; to: 80; duration: 200; easing.type: Easing.OutBack }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 }
    }

    background: Rectangle {
        color: root.type === 2 ? "#d32f2f" : (root.type === 1 ? "#f57c00" : Theme.panel)
        radius: 23
        border.color: root.type === 0 ? Theme.border : "transparent"
        border.width: 1

        // Тень для красоты
        layer.enabled: true
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 10

        TintedIcon {
            size: 20
            iconSource: root.type === 2 ? "qrc:/UpsNeyro2/icons/circle-x.svg"
                        : (root.type === 1 ? "qrc:/UpsNeyro2/icons/triangle-alert.svg"
                                           : "qrc:/UpsNeyro2/icons/info.svg")
            tint: "white"
        }

        Label {
            text: root.message
            color: "white"
            font.pixelSize: 14
            font.bold: true
        }
    }

    // Автоматически скрываем через 3 секунды
    Timer {
        id: hideTimer
        interval: 3000
        repeat: false
        onTriggered: root.close()
    }

    onOpened: hideTimer.start()
}
