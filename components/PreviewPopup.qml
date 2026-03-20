import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import UpsNeyro2 1.0

Popup {
    id: root
    width: 1100
    height: Math.min(800, Overlay.overlay.height - 100)
    anchors.centerIn: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    // popupType: Popup.Window

    property url    originalSource:  ""
    property url    upscaledSource:  ""
    property bool   processingBusy:  false
    property string processingText:  ""
    property int    processingValue: 0

    x: Math.round((Overlay.overlay.width  - width)  / 2)
    y: Math.round((Overlay.overlay.height - height) / 2)

    background: Rectangle {
        color: "#1f1f1f"
        radius: 14
        border.color: "#3a3a3a"
        border.width: 1
    }

    Overlay.modal: Rectangle {
        color: "#A6000000"
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    contentItem: Item {
        anchors.fill: parent
        anchors.margins: 12

        // ── Кнопка закрыть ────────────────────────────────────────────
        Button {
            id: closeBtn
            anchors.top:     parent.top
            anchors.right:   parent.right
            anchors.margins: 8
            width: 28; height: 28
            padding: 0; topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
            z: 10

            background: Rectangle {
                anchors.fill: parent
                color: closeBtn.hovered ? "#44e53935" : "transparent"
                radius: 6
                Behavior on color { ColorAnimation { duration: 150 } }
            }
            contentItem: Text {
                anchors.centerIn: parent
                text: "✖\uFE0E"
                color: Theme.textSecondary
                font.pixelSize: 14
            }
            onClicked: root.close()
        }

        // ── Основная колонка ──────────────────────────────────────────
        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            // ── Панели side-by-side ───────────────────────────────────
            Item {
                Layout.fillWidth:  true
                Layout.fillHeight: true

                // Оверлей со спиннером
                Rectangle {
                    anchors.fill: parent
                    color: "#cc1f1f1f"
                    radius: 10
                    visible: root.processingBusy
                    z: 5

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 16

                        Item {
                            Layout.alignment: Qt.AlignHCenter
                            width: 48; height: 48

                            Rectangle {
                                anchors.fill: parent
                                radius: width / 2
                                color: "transparent"
                                border.width: 4
                                border.color: Theme.accent
                                opacity: 0.25
                            }

                            Canvas {
                                id: spinnerCanvas
                                anchors.fill: parent

                                Connections {
                                    target: Theme
                                    function onAccentChanged() { spinnerCanvas.requestPaint() }
                                }

                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.beginPath()
                                    ctx.arc(width / 2, height / 2, width / 2 - 4,
                                            -Math.PI / 2, Math.PI * 0.7)
                                    ctx.strokeStyle = Theme.accent
                                    ctx.lineWidth   = 4
                                    ctx.lineCap     = "round"
                                    ctx.stroke()
                                }

                                RotationAnimator on rotation {
                                    running: root.processingBusy
                                    from: 0; to: 360
                                    duration: 900
                                    loops: Animation.Infinite
                                }
                            }
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.processingText !== "" ? root.processingText : "Processing..."
                            color: Theme.textPrimary
                            font.pixelSize: 16
                        }
                    }
                }

                // ── Левая панель: оригинал ────────────────────────────
                Rectangle {
                    x: 0; y: 0
                    width:  (parent.width - 12) / 2
                    height: parent.height
                    color: "#111111"
                    radius: 10
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: root.originalSource
                        fillMode: Image.PreserveAspectFit
                        visible: !root.processingBusy && root.originalSource.toString() !== ""
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "Original"
                        color: "#555"
                        font.pixelSize: 15
                        visible: root.originalSource.toString() === "" && !root.processingBusy
                    }

                    Rectangle {
                        anchors.bottom:           parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottomMargin: 10
                        width: origBadge.width + 20
                        height: 26; radius: 13
                        color: "#80000000"
                        visible: !root.processingBusy && root.originalSource.toString() !== ""

                        Label {
                            id: origBadge
                            anchors.centerIn: parent
                            text: "Original"
                            color: "white"
                            font.pixelSize: 12
                        }
                    }
                }

                // ── Разделитель ───────────────────────────────────────
                Rectangle {
                    width: 2
                    anchors.top:    parent.top
                    anchors.bottom: parent.bottom
                    x: parent.width / 2 - 1
                    color: "#ffffff40"
                }

                // ── Правая панель: апскейл ────────────────────────────
                Rectangle {
                    x: (parent.width - 12) / 2 + 12; y: 0
                    width:  (parent.width - 12) / 2
                    height: parent.height
                    color: "#111111"
                    radius: 10
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: root.upscaledSource
                        fillMode: Image.PreserveAspectFit
                        visible: !root.processingBusy && root.upscaledSource.toString() !== ""
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "AI Upscaled"
                        color: "#555"
                        font.pixelSize: 15
                        visible: root.upscaledSource.toString() === "" && !root.processingBusy
                    }

                    Rectangle {
                        anchors.bottom:           parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottomMargin: 10
                        width: upBadge.width + 20
                        height: 26; radius: 13
                        color: "#80000000"
                        visible: !root.processingBusy && root.upscaledSource.toString() !== ""

                        Label {
                            id: upBadge
                            anchors.centerIn: parent
                            text: "AI Upscaled"
                            color: Theme.accent
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }
            }

            // ── Статус-бар ────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                height: 42
                radius: 8
                color: "#2a2a32"
                visible: root.processingBusy || root.processingText !== ""

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin:  14
                    anchors.rightMargin: 14
                    spacing: 12

                    Label {
                        id: statusLabel
                        Layout.fillWidth: true
                        color: root.processingBusy ? Theme.textPrimary : Theme.textSecondary
                        font.pixelSize: 13

                        property int dotCount: 0

                        Timer {
                            running: root.processingBusy
                            interval: 400
                            repeat: true
                            onTriggered: statusLabel.dotCount = (statusLabel.dotCount + 1) % 4
                        }

                        text: {
                            var base = root.processingText !== "" ? root.processingText : "Processing"
                            return root.processingBusy
                                ? base + ".".repeat(statusLabel.dotCount)
                                : base
                        }
                    }

                    Label {
                        text: root.processingValue + "%"
                        color: Theme.accent
                        font.pixelSize: 13
                        font.bold: true
                        visible: root.processingBusy
                        Layout.preferredWidth: 40
                        horizontalAlignment: Text.AlignRight
                    }

                    Rectangle {
                        Layout.preferredWidth: 200
                        height: 6; radius: 3
                        color: "#3a3a44"
                        visible: root.processingBusy

                        Rectangle {
                            width: parent.width * (root.processingValue / 100)
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
            }
        }
    }
}
