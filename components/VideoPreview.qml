// VideoPreview.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import UpsNeyro2 1.0

Rectangle {
    id: root
    color: Theme.panel
    radius: 8
    clip: true

    // Переменная, где будет храниться путь к выбранному видео
    property string selectedVideoPath: ""
    property url videoUrl: ""

    // Диалоговое окно выбора файла
    FileDialog {
        id: videoDialog
        title: "Select a Video File"
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mov)", "All files (*)"]
        onAccepted: {
            root.videoUrl = selectedFile
            root.selectedVideoPath = selectedFile.toString().replace(/^(file:\/{2,3})/, "")
        }
    }

    DropArea {
        anchors.fill: parent

        onEntered: (drag) => {
            if (drag.hasUrls){
                drag.accept(Qt.LinkAction)
            } else {
                drag.ignore()
            }
        }

        onDropped: (drop) => {
            if(drop.hasUrls && drop.urls.length > 0){
                root.videoUrl = drop.urls[0]
                root.selectedVideoPath = drop.urls[0].toString().replace(/^(file:\/{2,3})/, "")
            }
        }
    }

    MediaPlayer {
        id: player
        source: root.videoUrl
        videoOutput: videoOutput

        // Когда видео готово, показываем первый кадр
        onMediaStatusChanged: {
            if (status === MediaPlayer.LoadedMedia) {
                player.play()
                player.pause()
            }
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        // Отображаем видео только если путь не пустой
        visible: root.videoUrl !== ""
        fillMode: VideoOutput.PreserveAspectFit
    }

    // text + click
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        // onClicked: videoDialog.open()
        onClicked: {
            if(root.videoUrl === ""){
                videoDialog.open()
            } else {
                if(player.playbackState === MediaPlayer.PlayingState)
                    player.pause()
                else
                    player.play()
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: (parent.containsMouse || root.DropArea.containsDrag) ? 0.05 : 0
            radius: 8
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 10
        visible: root.videoUrl === ""

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            // text: root.selectedVideoPath === "" ? "Video Preview" : "Selected Video:"
            text: "🎬"
            color: Theme.textSecondary
            font.pixelSize: 18
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Drop your video file here"
            color: Theme.textPrimary
            font.pixelSize: 18
            font.bold: true
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "or click to browse"
            color: Theme.textSecondary
        }
    }
}
