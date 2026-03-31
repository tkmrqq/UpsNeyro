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
    property string selectedVideoPath: ""
    property url    videoUrl: ""
    required property RecentFilesModel recentFiles

    signal videoLoaded(string path)

    function loadVideo(url) {
        if (!url || url.toString() === "") return
        videoUrl = url
        selectedVideoPath = url.toString().replace(/^file:\/{2,3}/, "")
        console.log("Load video:", videoUrl, "path:", selectedVideoPath)
        videoLoaded(url.toString())   // ← передаём полный URL, не путь
    }

    // ── Диалог выбора файла ───────────────────────────────────────────────────
    FileDialog {
        id: videoDialog
        title: "Select a Video File"
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.mov)", "All files (*)"]
        onAccepted: {
            if (selectedFile !== "")
                root.loadVideo(selectedFile)
        }
    }

    // ── Drag & Drop ───────────────────────────────────────────────────────────
    DropArea {
        id: dropArea
        anchors.fill: parent
        onEntered: (drag) => drag.hasUrls ? drag.accept(Qt.LinkAction) : drag.ignore()
        onDropped: (drop) => {
            if (drop.hasUrls && drop.urls.length > 0)
                root.loadVideo(drop.urls[0])
        }
    }

    // ── Плеер ─────────────────────────────────────────────────────────────────
    MediaPlayer {
        id: player
        source: root.videoUrl
        videoOutput: videoOut
        loops: MediaPlayer.Infinite

        audioOutput: AudioOutput {
            id: audioOut
            volume: 1.0
        }

        onMediaStatusChanged: {
            console.log("Media status:", player.mediaStatus, "hasVideo:", player.hasVideo)
        }
    }

    // ── Вывод видео ───────────────────────────────────────────────────────────
    VideoOutput {
        id: videoOut
        anchors.fill: parent
        visible: root.videoUrl.toString() !== ""
        fillMode: VideoOutput.PreserveAspectFit
    }

    // ── Плейсхолдер ──────────────────────────────────────────────────────────────
    Item {
        anchors.fill: parent
        visible: root.videoUrl.toString() === ""

        // Клик по всей зоне (кроме кнопки) → диалог
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: videoDialog.open()
        }

        // Контент по центру
        Column {
            anchors.centerIn: parent
            spacing: 12

            Label {
                text: "🎬"
                font.pixelSize: 40
                color: Theme.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: "Drop your video file here"
                font.pixelSize: 18
                font.bold: true
                color: Theme.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Label {
                text: "or click to browse"
                color: Theme.textSecondary
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // Кнопка Recent
            Rectangle {
                visible: recentFiles.files.length > 0
                anchors.horizontalCenter: parent.horizontalCenter
                width: 160; height: 32; radius: 8
                color: recentBtnMouse.containsMouse ? "#3a3a42" : "#2a2a32"
                z: 1   // ← выше MouseArea

                Behavior on color { ColorAnimation { duration: 120 } }

                Label {
                    anchors.centerIn: parent
                    text: "Recent Files ▾"
                    color: Theme.accent
                    font.pixelSize: 13
                }

                MouseArea {
                    id: recentBtnMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: {
                        mouse.accepted = true   // ← не пропускать клик вниз
                        recentMenu.popup()
                    }
                }

                Menu {
                    id: recentMenu
                    Repeater {
                        model: recentFiles.files
                        MenuItem {
                            text: {
                                let parts = modelData.split("/")
                                return parts[parts.length - 1]
                            }
                            onTriggered: root.loadVideo(Qt.url(modelData))
                        }
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: "Clear History"
                        onTriggered: recentFiles.clear()
                    }
                }
            }
        }
    }


    // ── Оверлей с кнопкой Play (видео загружено, но не играет) ───────────────
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        visible: root.videoUrl.toString() !== ""
                 && player.playbackState !== MediaPlayer.PlayingState

        // Затемнение только когда видео реально есть (не плейсхолдер)
        Rectangle {
            anchors.fill: parent
            color: "#40000000"
            visible: player.hasVideo
        }

        // Большая кнопка Play по центру
        Rectangle {
            anchors.centerIn: parent
            width: 72; height: 72
            radius: 36
            color: "#80000000"
            visible: player.hasVideo ||
                     player.mediaStatus === MediaPlayer.LoadedMedia ||
                     player.mediaStatus === MediaPlayer.BufferedMedia

            Behavior on opacity { NumberAnimation { duration: 150 } }
            opacity: playOverlayMouse.containsMouse ? 0.9 : 0.7

            // Треугольник Play
            Canvas {
                anchors.centerIn: parent
                width: 28; height: 28
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.beginPath()
                    ctx.moveTo(6, 2)
                    ctx.lineTo(6, 26)
                    ctx.lineTo(26, 14)
                    ctx.closePath()
                    ctx.fillStyle = "white"
                    ctx.fill()
                }
            }
        }

        MouseArea {
            id: playOverlayMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.videoUrl.toString() === "") {
                    videoDialog.open()
                } else {
                    player.play()
                }
            }
        }
    }

    // ── Клик по играющему видео — пауза ──────────────────────────────────────
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        visible: player.playbackState === MediaPlayer.PlayingState

        onClicked: player.pause()

        // Hover-подсветка
        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: parent.containsMouse ? 0.04 : 0
            radius: 8
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }
    }

    // ── Кнопка закрыть ────────────────────────────────────────────────────────
    Button {
        id: closeButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 15
        width: 24; height: 24
        z: 10
        padding: 0; topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
        visible: root.videoUrl.toString() !== ""

        background: Rectangle {
            color: closeButton.hovered ? "#cce53935" : "#80000000"
            radius: 8
            Behavior on color { ColorAnimation { duration: 150 } }
        }
        contentItem: Item {
            anchors.fill: parent
            Text {
                anchors.centerIn: parent
                text: "✖\uFE0E"
                color: "white"
            }
        }

        onClicked: {
            player.stop()
            root.videoUrl = ""
            root.selectedVideoPath = ""
        }
    }

    // ── Инфо-бейдж ────────────────────────────────────────────────────────────
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 15
        width: infoRow.width + 24
        height: 36
        color: "#80000000"
        radius: 18
        z: 10
        visible: root.videoUrl.toString() !== "" && player.hasVideo

        RowLayout {
            id: infoRow
            anchors.centerIn: parent
            spacing: 12

            Label {
                text: {
                    let path = root.videoUrl.toString()
                    if (path === "") return ""
                    let ext = path.split('.').pop().toUpperCase()
                    return ext.length <= 4 ? ext : "MEDIA"
                }
                color: Theme.accent
                font.pixelSize: 13
                font.bold: true
            }

            Label { text: "•"; color: Theme.textSecondary }

            Label {
                text: Math.round(videoOut.sourceRect.width) + "x"
                      + Math.round(videoOut.sourceRect.height)
                color: "white"; font.pixelSize: 13; font.bold: true
            }

            Label {
                text: "•"; color: Theme.textSecondary
                visible: codecLabel.text !== ""
            }
            Label {
                id: codecLabel
                text: (player.metaData && player.metaData.videoCodec)
                      ? player.metaData.videoCodec : ""
                color: "white"; font.pixelSize: 13
                visible: text !== ""
            }

            Label {
                text: "•"; color: Theme.textSecondary
                visible: fpsLabel.text !== ""
            }
            Label {
                id: fpsLabel
                text: (player.metaData && player.metaData.videoFrameRate)
                      ? Math.round(player.metaData.videoFrameRate) + " FPS" : ""
                color: "white"; font.pixelSize: 13
                visible: text !== ""
            }

            Label {
                text: "•"; color: Theme.textSecondary
                visible: player.hasAudio
            }
            Label {
                text: "Audio"; color: "white"
                font.pixelSize: 12; visible: player.hasAudio
            }
        }
    }
}
