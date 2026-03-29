import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Rectangle {
    color: Theme.panel
    radius: 8

    required property JobQueue jobQueue

    // ── Хелпер: цвет и текст статуса ────────────────────────────────
    function statusColor(status) {
        switch (status) {
        case ExportJob.Running:   return Theme.accent
        case ExportJob.Done:      return "#4caf50"
        case ExportJob.Failed:    return "#e53935"
        case ExportJob.Cancelled: return Theme.textSecondary
        default:                  return Theme.textSecondary  // Queued
        }
    }

    function statusLabel(status) {
        switch (status) {
        case ExportJob.Queued:    return "В очереди"
        case ExportJob.Running:   return "Обработка"
        case ExportJob.Done:      return "Готово"
        case ExportJob.Failed:    return "Ошибка"
        case ExportJob.Cancelled: return "Отменено"
        default:                  return "—"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // ── Заголовок + счётчик ──────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Export Queue"
                font.pixelSize: 22
                color: Theme.textPrimary
                Layout.fillWidth: true
            }

            // Бейдж с количеством незавершённых
            Rectangle {
                visible: jobQueue.pending > 0
                width: pendingLabel.implicitWidth + 16
                height: 24
                radius: 12
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)

                Label {
                    id: pendingLabel
                    anchors.centerIn: parent
                    text: jobQueue.pending
                    color: Theme.accent
                    font.pixelSize: 12
                    font.bold: true
                }
            }
        }

        // ── Кнопки управления ────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Старт / Отмена всего
            PrimaryButton {
                text: jobQueue.running ? "Отменить всё" : "Старт"
                enabled: jobQueue.running || jobQueue.pending > 0
                Layout.fillWidth: true

                background: Rectangle {
                    radius: 10
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: jobQueue.running ? "#c0392b" : Theme.accentGradientStart
                        }
                        GradientStop {
                            position: 1
                            color: jobQueue.running ? "#922b21" : Theme.accentGradientEnd
                        }
                    }
                }

                onClicked: {
                    if (jobQueue.running) jobQueue.cancelAll()
                    else                  jobQueue.start()
                }
            }

            // Пропустить текущее
            PrimaryButton {
                text: "Пропустить"
                enabled: jobQueue.running
                Layout.preferredWidth: 110

                onClicked: jobQueue.cancelCurrent()
            }

            // Очистить завершённые
            PrimaryButton {
                text: "Очистить"
                enabled: !jobQueue.running && jobQueue.jobs.length > 0
                Layout.preferredWidth: 100

                onClicked: jobQueue.clearFinished()
            }
        }

        // ── Список задач ─────────────────────────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            ListView {
                id: jobList
                width: parent.width
                model: jobQueue.jobs
                spacing: 8
                clip: true

                // Empty state
                Label {
                    anchors.centerIn: parent
                    visible: jobQueue.jobs.length === 0
                    text: "Нет задач"
                    color: Theme.textSecondary
                    font.pixelSize: 14
                }

                delegate: Rectangle {
                    id: jobCard
                    required property var modelData
                    required property int index

                    width: ListView.view.width
                    height: cardColumn.implicitHeight + 20
                    radius: 8
                    color: jobQueue.current === index ? "#2a2a3a" : "#2a2a32"

                    // Подсветка активного задания
                    Rectangle {
                        visible: jobQueue.current === index
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 3
                        radius: 2
                        color: Theme.accent
                    }

                    ColumnLayout {
                        id: cardColumn
                        anchors {
                            left: parent.left; right: parent.right
                            top: parent.top
                            margins: 10
                            leftMargin: 16
                        }
                        spacing: 6

                        // Имя файла + статус + кнопки
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            // Имя файла
                            Label {
                                text: {
                                    var parts = jobCard.modelData.inputPath.split("/")
                                    return parts[parts.length - 1]
                                }
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            // Статус-тег
                            Rectangle {
                                width: statusTagLabel.implicitWidth + 14
                                height: 20
                                radius: 10
                                color: Qt.rgba(
                                    statusColor(jobCard.modelData.status).r,
                                    statusColor(jobCard.modelData.status).g,
                                    statusColor(jobCard.modelData.status).b,
                                    0.18
                                )

                                Label {
                                    id: statusTagLabel
                                    anchors.centerIn: parent
                                    text: statusLabel(jobCard.modelData.status)
                                    color: statusColor(jobCard.modelData.status)
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            // ↑ ↓ ✕
                            RowLayout {
                                spacing: 2
                                visible: jobCard.modelData.status === ExportJob.Queued

                                // Вверх
                                Button {
                                    width: 22; height: 22
                                    padding: 0; topInset: 0; bottomInset: 0
                                    leftInset: 0; rightInset: 0
                                    enabled: jobCard.index > 0

                                    background: Rectangle {
                                        color: parent.hovered ? "#44ffffff" : "transparent"
                                        radius: 4
                                    }
                                    contentItem: Text {
                                        text: "↑"
                                        color: parent.enabled ? Theme.textSecondary : "#444"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: jobQueue.moveUp(jobCard.index)
                                }

                                // Вниз
                                Button {
                                    width: 22; height: 22
                                    padding: 0; topInset: 0; bottomInset: 0
                                    leftInset: 0; rightInset: 0
                                    enabled: jobCard.index < jobQueue.jobs.length - 1

                                    background: Rectangle {
                                        color: parent.hovered ? "#44ffffff" : "transparent"
                                        radius: 4
                                    }
                                    contentItem: Text {
                                        text: "↓"
                                        color: parent.enabled ? Theme.textSecondary : "#444"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: jobQueue.moveDown(jobCard.index)
                                }

                                // Удалить
                                Button {
                                    width: 22; height: 22
                                    padding: 0; topInset: 0; bottomInset: 0
                                    leftInset: 0; rightInset: 0

                                    background: Rectangle {
                                        color: parent.hovered ? "#44e53935" : "transparent"
                                        radius: 4
                                    }
                                    contentItem: Text {
                                        text: "✕"
                                        color: Theme.textSecondary
                                        font.pixelSize: 11
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: jobQueue.removeJob(jobCard.index)
                                }
                            }
                        }

                        // Прогресс-бар (только для Running)
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            visible: jobCard.modelData.status === ExportJob.Running

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: jobCard.modelData.statusText
                                    color: Theme.textSecondary
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: jobCard.modelData.eta !== "" ? "ETA " + jobCard.modelData.eta : ""
                                    color: Theme.textSecondary
                                    font.pixelSize: 11
                                    visible: jobCard.modelData.eta !== ""
                                }
                                Label {
                                    text: jobCard.modelData.progress + "%"
                                    color: Theme.accent
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 4
                                radius: 2
                                color: "#3a3a44"

                                Rectangle {
                                    width: parent.width * (jobCard.modelData.progress / 100)
                                    height: parent.height
                                    radius: parent.radius
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0.0; color: Theme.accentGradientStart }
                                        GradientStop { position: 1.0; color: Theme.accentGradientEnd }
                                    }
                                    Behavior on width { NumberAnimation { duration: 200 } }
                                }
                            }
                        }

                        // Путь к результату (Done)
                        Label {
                            visible: jobCard.modelData.status === ExportJob.Done &&
                                     jobCard.modelData.outputPath !== ""
                            text: "→ " + jobCard.modelData.outputPath
                            color: "#4caf50"
                            font.pixelSize: 11
                            elide: Text.ElideLeft
                            Layout.fillWidth: true
                        }

                        // Сообщение об ошибке (Failed)
                        Label {
                            visible: jobCard.modelData.status === ExportJob.Failed
                            text: jobCard.modelData.statusText
                            color: "#e53935"
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
