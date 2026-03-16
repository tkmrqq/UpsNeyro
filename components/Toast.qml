import QtQuick
import QtQuick.Controls
import UpsNeyro2 1.0

Popup {
    id: root

    property string message: ""
    property int type: 0 // 0 = Info, 1 = Warning, 2 = Error

    // Всплывает сверху по центру
    y: 80
    // anchors.horizontalCenter: parent.horizontalCenter
    anchors.centerIn: parent.horizontalCenter

    width: contentRow.width + 40
    height: 46

    // Не блокирует интерфейс!
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose

    // Анимация плавного появления сверху
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        NumberAnimation { property: "y"; from: 0; to: 40; duration: 200; easing.type: Easing.OutBack }
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

        Label {
            text: root.type === 2 ? "❌" : (root.type === 1 ? "⚠️" : "ℹ️")
            color: "white"
            font.pixelSize: 14
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
