import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0
import QtMultimedia

Rectangle {
    implicitHeight: 80
    radius: 8
    color: Theme.panel

    //линкуем плеер
    property var targetPlayer: null

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Button {
            // Меняем иконку
            text: (targetPlayer && targetPlayer.playbackState === MediaPlayer.PlayingState) ? "⏸" : "▶"
            enabled: targetPlayer && targetPlayer.hasVideo // Активна только если загружено видео
            onClicked: {
                if (targetPlayer) {
                    if (targetPlayer.playbackState === MediaPlayer.PlayingState) {
                        targetPlayer.pause()
                    } else {
                        targetPlayer.play()
                    }
                }
            }
        }

        Slider {
            id: timeSlider
            Layout.fillWidth: true
            enabled: targetPlayer && targetPlayer.hasVideo && targetPlayer.seekable

            // Диапазон от 0 до длительности видео
            from: 0
            to: targetPlayer ? targetPlayer.duration : 1
            // Текущее значение привязано к позиции плеера
            value: targetPlayer ? targetPlayer.position : 0

            // Когда пользователь перетаскивает ползунок — перематываем видео
            onMoved: {
                if (targetPlayer) {
                    targetPlayer.position = timeSlider.value
                }
            }
        }

    // Текст с таймкодом (текущее время / общее время)
        Label {
            Layout.preferredWidth: 100
            horizontalAlignment: Text.AlignRight
            color: Theme.textPrimary
            font.pixelSize: 14

            // Функция для форматирования миллисекунд в mm:ss
            function formatTime(ms) {
                if (!ms) return "00:00"
                let totalSeconds = Math.floor(ms / 1000)
                let minutes = Math.floor(totalSeconds / 60)
                let seconds = totalSeconds % 60
                return minutes.toString().padStart(2, '0') + ":" + seconds.toString().padStart(2, '0')
            }
            text: targetPlayer ? (formatTime(targetPlayer.position) + " / " + formatTime(targetPlayer.duration)) : "00:00 / 00:00"
        }

        // Слайдер громкости (второй слайдер в твоем коде)
        Label { text: "🔊"; color: Theme.textSecondary }
            Slider {
                id: volumeSlider
                Layout.preferredWidth: 100
                from: 0
                to: 1
                value: targetPlayer ? targetPlayer.audioOutput.volume : 0.5
                onValueChanged: {
                    if (targetPlayer && targetPlayer.audioOutput) {
                        targetPlayer.audioOutput.volume = value
                    }
                }
            }
        }
}
