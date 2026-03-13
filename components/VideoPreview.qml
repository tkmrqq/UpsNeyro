// VideoPreview.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
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
        selectedVideoPath = url.toString().replace(/^(file:\/{2,3})/, "")

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
        audioOutput: AudioOutput {}
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

            if (player.mediaStatus === MediaPlayer.LoadedMedia) {
                console.log("Video Codec:", player.metaData.videoCodec)
                console.log("FPS:", player.metaData.videoFrameRate)
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

    //close button
    Button {
        id: closeButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 15
        width: 36
        height: 36
        z: 10

        visible: root.videoUrl.toString() !== ""

        // Кастомный дизайн (полупрозрачный круглый фон)
        background: Rectangle {
            color: closeButton.hovered ? "#cce53935" : "#80000000" // Краснеет при наведении
            radius: 18
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        contentItem: Text {
            text: "✖\uFE0E"
            color: "white"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        onClicked: {
            player.stop()            // Останавливаем плеер
            root.videoUrl = ""       // Очищаем URL (текст появится сам)
            root.selectedVideoPath = "" // Очищаем путь
            root.hasCapturedFirstFrame = false
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 15

        // Динамический размер в зависимости от длины текста
        width: infoRow.width + 24
        height: 36
        color: "#80000000" // Полупрозрачный черный
        radius: 18
        z: 10

        // Показываем, только если видео реально готово и отрендерено
        visible: root.videoUrl.toString() !== "" && player.hasVideo

        RowLayout {
            id: infoRow
            anchors.centerIn: parent
            spacing: 12

            // 1. format
            Label {
                text: {
                    let path = root.videoUrl.toString()
                    if (path === "") return ""
                    let parts = path.split('.')
                    let ext = parts.length > 1 ? parts.pop().toUpperCase() : "VIDEO"
                    return ext.length <= 4 ? ext : "MEDIA"
                }
                color: Theme.accent
                font.pixelSize: 13
                font.bold: true
            }

            Label { text: "•"; color: Theme.textSecondary }

            // 2. resolution
            Label {
                text: Math.round(videoOut.sourceRect.width) + "x" + Math.round(videoOut.sourceRect.height)
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }
            // 3. codec
            Label {
                text: "•"
                color: Theme.textSecondary
                visible: codecLabel.text !== ""
            }
            Label {
                id: codecLabel
                text: (player.metaData && player.metaData.videoCodec) ? player.metaData.videoCodec : ""
                color: "white"
                font.pixelSize: 13
                visible: text !== ""
            }

            // 4. FPS
            Label {
                text: "•"
                color: Theme.textSecondary
                visible: fpsLabel.text !== ""
            }
            Label {
                id: fpsLabel
                text: (player.metaData && player.metaData.videoFrameRate) ? Math.round(player.metaData.videoFrameRate) + " FPS" : ""
                color: "white"
                font.pixelSize: 13
                visible: text !== ""
            }

            // 5. Sound
            Label {
                text: "•"
                color: Theme.textSecondary
                visible: player.hasAudio
            }

            Label {
                text: "Audio"
                color: "white"
                font.pixelSize: 12
                visible: player.hasAudio
            }
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
