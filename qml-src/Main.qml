import QtQuick
import QtQuick.Controls

ApplicationWindow {
    height: 480
    title: "I2C-scanner"
    visible: true
    width: 800

    // Обработка сигналов через connect
    Component.onCompleted: {
        serialManager.errorOccurred.connect(function (msg) {
            console.log("Serial Error: " + msg);
        });
    }

    // SerialManager должен быть зарегистрирован в контексте C++:
    // engine.rootContext()->setContextProperty("serialManager", &serialManager);

    Column {
        anchors.centerIn: parent
        spacing: 10

        Row {
            spacing: 10

            ComboBox {
                id: portSelector

                model: serialManager.ports
                width: 200
                onCurrentTextChanged: serialManager.portName = currentText
            }
            Button {
                id: openButton

                text: serialManager.isOpen ? "Port Open" : "Open Port"
                width: 90
                background: Rectangle {
                    color: serialManager.isOpen ? "green" : "red"
                    radius: 6
                }

                onClicked: {
                    if (serialManager.isOpen)
                        serialManager.closePort();
                    else
                        serialManager.openPort();
                }
            }
        }
        TextArea {
            id: logArea

            height: 150
            readOnly: true
            text: serialManager.receivedData
            width: 300
        }
        Row {
            spacing: 10

            TextField {
                id: inputField

                placeholderText: "Send text"
                width: 200
            }
            Button {
                text: "Send"

                width: 90
                onClicked: serialManager.sendData(inputField.text)
            }
        }
    }
}
