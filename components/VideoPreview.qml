// VideoPreview.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Effects
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

    function fileBasenameForPath(p) {
        if (!p)
            return ""
        var s = String(p).replace(/\\/g, "/")
        var i = s.lastIndexOf("/")
        return i >= 0 ? s.slice(i + 1) : s
    }

    /** Публичный вход для восстановления пути из сессии / ProjectManager. */
    function loadLocalPath(path) {
        loadVideoFromLocalPath(path)
    }

    // Путь с диска (из Recent) → QUrl; Menu+Repeater на Windows давал зависания
    function loadVideoFromLocalPath(path) {
        if (!path)
            return
        var p = String(path).trim()
        if (p.indexOf("file:") === 0) {
            root.loadVideo(Qt.url(p))
            return
        }
        var norm = p.replace(/\\/g, "/")
        var fileUrl = (norm.length >= 2 && norm.charAt(1) === ":")
                      ? ("file:///" + norm)
                      : ("file://" + norm)
        root.loadVideo(Qt.url(fileUrl))
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

            // Recent files — кастомный Popup вместо Menu (стиль + без подвисаний)
            Rectangle {
                id: recentFilesAnchor
                visible: recentFiles.files.length > 0
                anchors.horizontalCenter: parent.horizontalCenter
                width: 168
                height: 34
                radius: 8
                color: recentBtnMouse.containsMouse ? "#3a3a42" : "#2a2a32"
                border.width: 1
                border.color: recentBtnMouse.containsMouse ? Theme.border : "#33333d"
                z: 100

                Behavior on color { ColorAnimation { duration: 120 } }

                Row {
                    anchors.centerIn: parent
                    spacing: 6

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Recent files")
                        color: Theme.accent
                        font.pixelSize: 13
                    }
                    TintedIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        size: 14
                        iconSource: "qrc:/UpsNeyro2/icons/chevron-down.svg"
                        tint: Theme.accent
                    }
                }

                MouseArea {
                    id: recentBtnMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: {
                        mouse.accepted = true
                        recentFilesPopup.open()
                    }
                }

                Popup {
                    id: recentFilesPopup
                    parent: recentFilesAnchor
                    padding: 0
                    modal: false
                    focus: true
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                    width: Math.min(Math.max(recentFilesAnchor.width + 120, 280), 420)
                    height: Math.min(Math.max(recentPopupColumn.implicitHeight, 120), 360)

                    x: Math.round((recentFilesAnchor.width - width) / 2)
                    y: recentFilesAnchor.height + 8

                    background: Rectangle {
                        color: Theme.panel
                        radius: 10
                        border.color: Theme.border
                        border.width: 1
                    }

                    contentItem: ColumnLayout {
                        id: recentPopupColumn
                        width: recentFilesPopup.width
                        spacing: 0

                        Label {
                            text: qsTr("Open recent")
                            font.pixelSize: 11
                            font.bold: true
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Layout.topMargin: 10
                            Layout.bottomMargin: 6
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Theme.border
                        }

                        ListView {
                            id: recentListView
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(recentFiles.files.length * 40, 240)
                            clip: true
                            model: recentFiles.files
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: Rectangle {
                                required property var modelData

                                width: recentListView.width
                                height: 40
                                color: pathMa.containsMouse ? "#35353d" : "transparent"

                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    text: root.fileBasenameForPath(modelData)
                                    color: Theme.textPrimary
                                    font.pixelSize: 13
                                    elide: Text.ElideMiddle
                                }
                                MouseArea {
                                    id: pathMa
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onClicked: {
                                        recentFilesPopup.close()
                                        root.loadVideoFromLocalPath(modelData)
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Theme.border
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 42
                            color: clearMa.containsMouse ? "#3d2528" : "transparent"

                            Label {
                                anchors.centerIn: parent
                                text: qsTr("Clear history")
                                color: "#e57373"
                                font.pixelSize: 13
                            }
                            MouseArea {
                                id: clearMa
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onClicked: {
                                    recentFilesPopup.close()
                                    recentFiles.clear()
                                }
                            }
                        }
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

            Image {
                id: playOverlaySvg
                anchors.centerIn: parent
                source: "qrc:/UpsNeyro2/icons/play.svg"
                sourceSize.width: 32
                sourceSize.height: 32
                visible: false
            }
            MultiEffect {
                anchors.centerIn: parent
                width: 32
                height: 32
                source: playOverlaySvg
                brightness: 1.0
                colorization: 1.0
                colorizationColor: "white"
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
            TintedIcon {
                anchors.centerIn: parent
                size: 14
                iconSource: "qrc:/UpsNeyro2/icons/x.svg"
                tint: "white"
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
