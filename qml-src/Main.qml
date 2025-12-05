import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 800
    height: 480
    title: "COM-port demo"

    Column {
        anchors.centerIn: parent
        spacing: 10

        ComboBox {
            id: portSelector
            model: serialManager.ports
            onCurrentTextChanged: serialManager.portName = currentText
        }

        Button {
            id: openButton
            text: serialManager.isOpen ? "Port Open" : "Open Port"
            background: Rectangle {
                color: serialManager.isOpen ? "green" : "red"
                radius: 6
            }
            onClicked: {
                if (serialManager.isOpen)
                    serialManager.closePort()
                else
                    serialManager.openPort()
            }
        }

        TextArea {
            id: logArea
            width: 350
            height: 150
            readOnly: true
            text: serialManager.receivedData
        }

        Row {
            spacing: 5
            TextField {
                id: inputField
                width: 200
                placeholderText: "Send text"
            }
            Button {
                text: "Send"
                onClicked: serialManager.sendData(inputField.text)
            }
        }
    }

    Connections {
        target: serialManager
        onErrorOccurred: console.log("Serial Error: " + msg)
    }
}
