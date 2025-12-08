import QtQuick
import QtQuick.Controls

ApplicationWindow {
    height: 480
    title: "I2C-scanner"
    visible: true
    width: 800

    SI7210Window {
        id: siWindow
    }

    Component.onCompleted: i2cScanner.refreshDevices()

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Row {
            spacing: 8

            Button {
                text: "Окно SI7210"
                onClicked: siWindow.visible = true
            }
        }

        Row {
            spacing: 8

            Button {
                text: "Обновить"
                onClicked: i2cScanner.refreshDevices()
            }

            ComboBox {
                id: deviceSelector
                width: 360
                model: i2cScanner.devices
                onCurrentIndexChanged: i2cScanner.selectedIndex = currentIndex
            }
        }

        Row {
            spacing: 8

            TextField {
                id: startField
                width: 70
                placeholderText: "03"
                text: "03"
                inputMask: "HH"
            }

            TextField {
                id: endField
                width: 70
                placeholderText: "77"
                text: "77"
                inputMask: "HH"
            }

            Button {
                text: "Сканировать"
                onClicked: {
                    const start = parseInt(startField.text || "03", 16)
                    const end = parseInt(endField.text || "77", 16)
                    i2cScanner.scan(start, end)
                }
            }
        }

        Row {
            spacing: 8

            Label { text: "Ведомый:" }
            ComboBox {
                id: slaveSelector
                width: 120
                model: ["0x3C", "0x3D", "0x27", "0x20"]
                currentIndex: 0
                onCurrentTextChanged: i2cScanner.selectedSlave = currentText
            }

            Label { text: "Смещение:" }
            TextField {
                id: memOffset
                width: 70
                placeholderText: "00"
                text: "00"
                inputMask: "HH"
            }

            Label { text: "Длина:" }
            TextField {
                id: memLength
                width: 60
                placeholderText: "10"
                text: "10"
                inputMask: "HH"
            }

            Button {
                text: "Читать"
                onClicked: {
                    const addr = parseInt(slaveSelector.currentText, 16)
                    const offset = parseInt(memOffset.text || "00", 16)
                    const len = parseInt(memLength.text || "10", 16)
                    const data = i2cScanner.readMemory(addr, offset, len)
                    if (data && data.length > 0) {
                        let hex = []
                        for (let i = 0; i < data.length; ++i) {
                            const b = data.at ? data.at(i) : data[i]
                            const v = typeof b === "number" ? b : b.charCodeAt(0)
                            hex.push(("0" + v.toString(16)).slice(-2))
                        }
                        i2cScanner.setSelectedSlave(slaveSelector.currentText)
                        console.log("Read:", hex.join(" "))
                    }
                }
            }

            TextField {
                id: writeData
                width: 180
                placeholderText: "Напр. A0 FF 12"
            }

            Button {
                text: "Писать"
                onClicked: {
                    const addr = parseInt(slaveSelector.currentText, 16)
                    const offset = parseInt(memOffset.text || "00", 16)
                    const bytes = writeData.text.split(/[\s,]+/)
                        .filter(x => x.length > 0)
                        .map(x => parseInt(x, 16))
                        .filter(x => !isNaN(x))
                    const arr = new Uint8Array(bytes)
                    i2cScanner.writeMemory(addr, offset, arr)
                }
            }
        }

        GroupBox {
            title: "Лог"
            width: parent.width
            height: 200

            TextArea {
                anchors.fill: parent
                readOnly: true
                wrapMode: TextArea.WrapAnywhere
                text: i2cScanner.log
                font.family: "monospace"
            }
        }
    }
}
