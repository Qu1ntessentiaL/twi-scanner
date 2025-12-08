import QtQuick
import QtQuick.Controls

Window {
    id: siWindow
    width: 560
    height: 480
    title: "SI7210 Monitor"
    visible: false

    signal requestClose()

    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Row {
            spacing: 8

            Label { text: "Адрес SI7210:" }
            TextField {
                id: siAddress
                width: 70
                text: "33"       // по умолчанию 0x33
                inputMask: "HH"
            }

            Label { text: "Start:" }
            TextField {
                id: regStart
                width: 70
                text: "C8"
                inputMask: "HH"
            }

            Label { text: "Len:" }
            TextField {
                id: regLen
                width: 70
                text: "02"
                inputMask: "HH"
            }

            Button {
                text: "Читать"
                onClicked: {
                    const addr = parseInt(siAddress.text || "33", 16)
                    const start = parseInt(regStart.text || "00", 16)
                    const len = parseInt(regLen.text || "10", 16)
                    const hex = i2cScanner.readRegistersHex(addr, start, len)
                    if (hex.length > 0) {
                        logArea.append(`0x${siAddress.text} [${regStart.text}..${(start+len-1).toString(16)}]: ${hex}`)
                    }
                }
            }
        }

        GroupBox {
            title: "Быстрые регистры SI7210"
            width: parent.width
            height: 150

            Column {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Button {
                    text: "Читать статус (0xC0, 8 байт)"
                    onClicked: {
                        const addr = parseInt(siAddress.text || "33", 16)
                        const hex = i2cScanner.readRegistersHex(addr, 0xC0, 8)
                        if (hex.length > 0)
                            logArea.append(`Status 0xC0: ${hex}`)
                    }
                }

                Button {
                    text: "Читать поле данных (0xC8, 8 байт)"
                    onClicked: {
                        const addr = parseInt(siAddress.text || "33", 16)
                        const hex = i2cScanner.readRegistersHex(addr, 0xC8, 8)
                        if (hex.length > 0)
                            logArea.append(`Field 0xC8: ${hex}`)
                    }
                }
            }
        }

        GroupBox {
            title: "Лог"
            width: parent.width
            anchors.margins: 0
            height: 230

            ScrollView {
                anchors.fill: parent
                clip: true

                TextArea {
                    id: logArea
                    anchors.fill: parent
                    readOnly: true
                    wrapMode: TextArea.WrapAnywhere
                    font.family: "monospace"
                    textFormat: TextEdit.PlainText
                    onTextChanged: {
                        cursorPosition = length
                    }
                }
            }
        }

        Row {
            spacing: 12
            Label { text: "Авто‑опрос поля (0xC8, 2 байта):" }
            CheckBox {
                id: autoPoll
                text: autoTimer.running ? "Стоп" : "Старт"
                checked: false
                onCheckedChanged: {
                    autoTimer.running = checked
                }
            }
            Label { text: "Интервал, мс" }
            SpinBox {
                id: pollInterval
                from: 50
                to: 2000
                value: 200
                stepSize: 50
            }
        }

        Timer {
            id: autoTimer
            interval: pollInterval.value
            repeat: true
            running: false
            onTriggered: {
                const addr = parseInt(siAddress.text || "33", 16)
                const hex = i2cScanner.readRegistersHex(addr, 0xC8, 2)
                if (hex.length > 0) {
                    logArea.append(`AUTO 0xC8: ${hex}`)
                }
            }
        }
    }
}
