import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import UpsNeyro2 1.0

Rectangle {
    id: root
    color: Theme.panel
    radius: 8

    required property JobQueue jobQueue

    function folderPathFromUrl(u) {
        var s = u.toString()
        if (s.indexOf("file:") === 0) {
            s = s.replace(/^file:\/{2,3}/, "")
            if (Qt.platform.os === "windows" && s.length >= 3 && s.charAt(0) === "/"
                    && s.charAt(2) === ":")
                s = s.substring(1)
        }
        return s
    }

    FolderDialog {
        id: addFolderDialog
        title: qsTr("Add videos from folder")
        onAccepted: {
            jobQueue.addJobsFromDirectory(folderPathFromUrl(selectedFolder))
        }
    }

    function statusColor(status) {
        switch (status) {
        case ExportJob.Running:
            return Theme.accent
        case ExportJob.Done:
            return "#4caf50"
        case ExportJob.Failed:
            return "#e53935"
        case ExportJob.Cancelled:
            return Theme.textSecondary
        default:
            return Theme.textSecondary
        }
    }

    function statusLabel(status) {
        switch (status) {
        case ExportJob.Queued:
            return qsTr("Queued")
        case ExportJob.Running:
            return qsTr("Processing")
        case ExportJob.Done:
            return qsTr("Done")
        case ExportJob.Failed:
            return qsTr("Failed")
        case ExportJob.Cancelled:
            return qsTr("Cancelled")
        default:
            return "—"
        }
    }

    function fileBasename(path) {
        if (!path || path.length === 0)
            return ""
        var p = path.replace(/\\/g, "/")
        var parts = p.split("/")
        return parts[parts.length - 1]
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // ── Заголовок: название + счётчик слева, справа пусто под кнопку окна ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("Export queue")
                    font.pixelSize: 22
                    color: Theme.textPrimary
                }

                Rectangle {
                    visible: jobQueue.pending > 0
                    implicitWidth: Math.max(pendingLabel.implicitWidth + 14, 28)
                    implicitHeight: 26
                    radius: 13
                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                    border.width: 1
                    border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35)

                    Label {
                        id: pendingLabel
                        anchors.centerIn: parent
                        text: jobQueue.pending
                        color: Theme.accent
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                }
            }

            Label {
                Layout.fillWidth: true
                leftPadding: 0
                text: {
                    if (jobQueue.running)
                        return qsTr("Processing the queue…")
                    if (jobQueue.jobs.length === 0)
                        return qsTr("Add videos via “Add to queue” on the Upscale tab.")
                    if (jobQueue.pending > 0)
                        return qsTr("%1 in queue").arg(jobQueue.pending)
                    return qsTr("Queue is idle.")
                }
                wrapMode: Text.WordWrap
                color: Theme.textSecondary
                font.pixelSize: 12
                lineHeight: 1.25
            }
        }

        // ── Actions (Rectangle + ColumnLayout с anchors.fill даёт высоту 0 — явный implicitHeight) ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: actionsColumn.implicitHeight + 28
            radius: 10
            color: "#222228"
            border.width: 1
            border.color: Theme.border

            ColumnLayout {
                id: actionsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 12

                Label {
                    text: qsTr("Actions")
                    font.pixelSize: 13
                    font.bold: true
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                }

                PrimaryButton {
                    id: primaryQueueBtn
                    text: jobQueue.running ? qsTr("Cancel all") : qsTr("Start queue")
                    enabled: jobQueue.running || jobQueue.pending > 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    topPadding: 0
                    bottomPadding: 0

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

                    contentItem: Text {
                        text: primaryQueueBtn.text
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    onClicked: {
                        if (jobQueue.running)
                            jobQueue.cancelAll()
                        else
                            jobQueue.start()
                    }
                }

                Button {
                    id: addFolderBtn
                    text: qsTr("Add folder…")
                    enabled: !jobQueue.running
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    flat: true
                    padding: 10
                    onClicked: addFolderDialog.open()
                    background: Rectangle {
                        radius: 8
                        color: addFolderBtn.enabled && addFolderBtn.hovered ? "#35353d" : "#2c2c34"
                        border.width: 1
                        border.color: Theme.border
                    }
                    contentItem: Text {
                        text: addFolderBtn.text
                        color: addFolderBtn.enabled ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        id: skipBtn
                        text: qsTr("Skip current")
                        enabled: jobQueue.running
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        flat: true
                        padding: 10
                        onClicked: jobQueue.cancelCurrent()

                        background: Rectangle {
                            radius: 8
                            color: skipBtn.enabled && skipBtn.hovered ? "#35353d" : "#2c2c34"
                            border.width: 1
                            border.color: Theme.border
                        }
                        contentItem: Text {
                            text: skipBtn.text
                            color: skipBtn.enabled ? Theme.textPrimary : Theme.textSecondary
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }

                    Button {
                        id: clearBtn
                        text: qsTr("Clear finished")
                        enabled: !jobQueue.running && jobQueue.jobs.length > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        flat: true
                        padding: 10
                        onClicked: jobQueue.clearFinished()

                        background: Rectangle {
                            radius: 8
                            color: clearBtn.enabled && clearBtn.hovered ? "#35353d" : "#2c2c34"
                            border.width: 1
                            border.color: Theme.border
                        }
                        contentItem: Text {
                            text: clearBtn.text
                            color: clearBtn.enabled ? Theme.textPrimary : Theme.textSecondary
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Label {
            text: qsTr("Jobs")
            font.pixelSize: 13
            font.bold: true
            color: Theme.textSecondary
            visible: jobQueue.jobs.length > 0
            Layout.fillWidth: true
            Layout.topMargin: 4
        }

        // ── Список ───────────────────────────────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ListView {
                id: jobList
                width: parent.width
                model: jobQueue.jobs
                spacing: 10
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                // Пустое состояние
                Item {
                    anchors.centerIn: parent
                    width: emptyCol.implicitWidth
                    height: emptyCol.implicitHeight
                    visible: jobQueue.jobs.length === 0

                    ColumnLayout {
                        id: emptyCol
                        spacing: 10

                        TintedIcon {
                            Layout.alignment: Qt.AlignHCenter
                            size: 40
                            iconSource: "qrc:/UpsNeyro2/icons/queue.svg"
                            tint: Theme.textSecondary
                            opacity: 0.55
                        }

                        Label {
                            text: qsTr("No jobs yet")
                            color: Theme.textPrimary
                            font.pixelSize: 15
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Label {
                            text: qsTr("Use “Add to Queue” on the Upscale tab.")
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: jobList.width - 24
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                delegate: Rectangle {
                    id: jobCard
                    required property var modelData
                    required property int index

                    width: ListView.view.width
                    height: cardColumn.implicitHeight + 24
                    radius: 10
                    color: jobQueue.current === index ? "#2e2e38" : "#26262c"
                    border.width: jobQueue.current === index ? 1 : 1
                    border.color: jobQueue.current === index
                                  ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.55)
                                  : Theme.border

                    ColumnLayout {
                        id: cardColumn
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            margins: 12
                        }
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: fileBasename(jobCard.modelData.inputPath)
                                color: Theme.textPrimary
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                implicitWidth: statusTagLabel.implicitWidth + 16
                                implicitHeight: 22
                                radius: 11
                                color: Qt.rgba(
                                    statusColor(jobCard.modelData.status).r,
                                    statusColor(jobCard.modelData.status).g,
                                    statusColor(jobCard.modelData.status).b,
                                    0.14
                                )
                                border.width: 1
                                border.color: Qt.rgba(
                                    statusColor(jobCard.modelData.status).r,
                                    statusColor(jobCard.modelData.status).g,
                                    statusColor(jobCard.modelData.status).b,
                                    0.35
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
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            visible: jobCard.modelData.status === ExportJob.Queued

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                radius: 8
                                color: "#1a1a1f"
                                border.width: 1
                                border.color: Theme.border
                                height: 30
                                width: reorderRow.implicitWidth + 12

                                RowLayout {
                                    id: reorderRow
                                    anchors.centerIn: parent
                                    spacing: 0
                                    implicitHeight: 28

                                    Button {
                                        id: moveUpBtn
                                        width: 30
                                        height: 28
                                        padding: 0
                                        flat: true
                                        enabled: jobCard.index > 0
                                        hoverEnabled: true

                                        background: Rectangle {
                                            color: parent.hovered && parent.enabled ? "#35353d" : "transparent"
                                            radius: 6
                                        }
                                        contentItem: Item {
                                            TintedIcon {
                                                anchors.centerIn: parent
                                                size: 16
                                                iconSource: "qrc:/UpsNeyro2/icons/chevron-up.svg"
                                                tint: moveUpBtn.enabled ? Theme.textSecondary : "#555555"
                                            }
                                        }
                                        onClicked: jobQueue.moveUp(jobCard.index)
                                    }

                                    Rectangle {
                                        width: 1
                                        Layout.fillHeight: true
                                        Layout.preferredHeight: 18
                                        color: Theme.border
                                    }

                                    Button {
                                        id: moveDownBtn
                                        width: 30
                                        height: 28
                                        padding: 0
                                        flat: true
                                        enabled: jobCard.index < jobQueue.jobs.length - 1
                                        hoverEnabled: true

                                        background: Rectangle {
                                            color: parent.hovered && parent.enabled ? "#35353d" : "transparent"
                                            radius: 6
                                        }
                                        contentItem: Item {
                                            TintedIcon {
                                                anchors.centerIn: parent
                                                size: 16
                                                iconSource: "qrc:/UpsNeyro2/icons/chevron-down.svg"
                                                tint: moveDownBtn.enabled ? Theme.textSecondary : "#555555"
                                            }
                                        }
                                        onClicked: jobQueue.moveDown(jobCard.index)
                                    }

                                    Rectangle {
                                        width: 1
                                        Layout.fillHeight: true
                                        Layout.preferredHeight: 18
                                        color: Theme.border
                                    }

                                    Button {
                                        width: 30
                                        height: 28
                                        padding: 0
                                        flat: true
                                        hoverEnabled: true

                                        background: Rectangle {
                                            color: parent.hovered ? "#3d2528" : "transparent"
                                            radius: 6
                                        }
                                        contentItem: Item {
                                            TintedIcon {
                                                anchors.centerIn: parent
                                                size: 15
                                                iconSource: "qrc:/UpsNeyro2/icons/trash-2.svg"
                                                tint: "#c62828"
                                            }
                                        }
                                        onClicked: jobQueue.removeJob(jobCard.index)
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            visible: jobCard.modelData.status === ExportJob.Running

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Label {
                                    text: jobCard.modelData.statusText
                                    color: Theme.textSecondary
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: jobCard.modelData.eta !== "" ? ("ETA " + jobCard.modelData.eta) : ""
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
                                height: 5
                                radius: 2.5
                                color: "#1e1e24"

                                Rectangle {
                                    width: Math.max(0, parent.width * (jobCard.modelData.progress / 100))
                                    height: parent.height
                                    radius: parent.radius
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop {
                                            position: 0.0
                                            color: Theme.accentGradientStart
                                        }
                                        GradientStop {
                                            position: 1.0
                                            color: Theme.accentGradientEnd
                                        }
                                    }
                                    Behavior on width {
                                        NumberAnimation {
                                            duration: 200
                                        }
                                    }
                                }
                            }
                        }

                        Label {
                            visible: jobCard.modelData.status === ExportJob.Done
                                     && jobCard.modelData.outputPath !== ""
                            text: qsTr("Saved: %1").arg(jobCard.modelData.outputPath)
                            color: "#66bb6a"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        Label {
                            visible: jobCard.modelData.status === ExportJob.Failed
                            text: jobCard.modelData.statusText
                            color: "#ef5350"
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
