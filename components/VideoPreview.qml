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

    property alias mediaPlayer: player

    // Переменная, где будет храниться путь к выбранному видео
    property string selectedVideoPath: ""
    property url videoUrl: ""

    function loadVideo(url) {
        if (!url || url.toString() === "") return
        videoUrl = url
        selectedVideoPath = Qt.url.toLocalFile(url)
        console.log("Load video:", videoUrl, "path:", selectedVideoPath)
    }

    // Диалоговое окно выбора файла
    FileDialog {
        id: videoDialog
        title: "Select a Video File"
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mov)", "All files (*)"]
        onAccepted: {
            if (selectedFile !== "") {
                root.loadVideo(selectedFile)
            }
        }
    }

    DropArea {
        id: dropArea
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
        // audioOutput: AudioOutput {}
        source: root.videoUrl
        videoOutput: videoOut
        loops: MediaPlayer.Infinite

        // Когда видео готово, показываем первый кадр
        onMediaStatusChanged: {
            console.log("Media status:", player.mediaStatus)
            if (player.mediaStatus === MediaPlayer.BufferedMedia || player.mediaStatus === MediaPlayer.LoadedMedia) {
                if (player.position === 0) {
                    player.play()
                }
            }
        }

        //if pos >0 pausim
        onPositionChanged: {
            if (player.position > 0 && player.position < 500 && player.playbackState === MediaPlayer.PlayingState) {
                if (!root.hasCapturedFirstFrame) {
                    player.pause()
                    root.hasCapturedFirstFrame = true
                }
            }
        }

    }

    property bool hasCapturedFirstFrame: false

    onVideoUrlChanged: {
        hasCapturedFirstFrame = false
    }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
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
            if(root.videoUrl.toString() === "") {
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
            opacity: (parent.containsMouse || dropArea.containsDrag) ? 0.05 : 0
            radius: 8
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 10
        visible: root.videoUrl.toString() === ""

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            // text: root.selectedVideoPath === "" ? "Video Preview" : "Selected Video:"
            text: "🎬"
            color: Theme.textSecondary
            font.pixelSize: 40
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
