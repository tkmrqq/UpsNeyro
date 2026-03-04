// VideoPreview.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs // ДОБАВЛЕНО: для выбора файлов
import UpsNeyro2 1.0

Rectangle {
    id: root
    color: Theme.panel
    radius: 8

    // Переменная, где будет храниться путь к выбранному видео
    property string selectedVideoPath: ""

    // Диалоговое окно выбора файла
    FileDialog {
        id: videoDialog
        title: "Select a Video File"
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mov)", "All files (*)"]
        onAccepted: {
            // Убираем приставку file:/// для красивого отображения пути
            root.selectedVideoPath = selectedFile.toString().replace(/^(file:\/{2,3})/, "")
        }
    }

    // Зона клика, занимающая весь прямоугольник
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor // Курсор-палец при наведении
        onClicked: videoDialog.open()

        // Легкая подсветка при наведении мыши (визуальный фидбек)
        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: parent.containsMouse ? 0.03 : 0
            radius: 8
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            // Меняем текст, если файл выбран
            text: root.selectedVideoPath === "" ? "Video Preview" : "Selected Video:"
            color: Theme.textSecondary
            font.pixelSize: 18
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            // Показываем путь к файлу вместо подсказки
            text: root.selectedVideoPath === "" ? "Drop your video file here or click to browse" : root.selectedVideoPath
            color: root.selectedVideoPath === "" ? Theme.textSecondary : Theme.textPrimary
            font.bold: root.selectedVideoPath !== ""
            width: root.width - 40
            wrapMode: Text.Wrap // Перенос длинного пути на новую строку
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
