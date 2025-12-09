import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: app

    function applySystemNow() {
        const d = new Date();
        yearBox.value = d.getUTCFullYear();
        monthBox.value = d.getUTCMonth() + 1;
        dayBox.value = d.getUTCDate();
        hourBox.value = d.getUTCHours();
        minuteBox.value = d.getUTCMinutes();
        secondBox.value = d.getUTCSeconds();
    }

    height: 680
    title: "RTC I2C Utility"
    visible: true
    width: 900

    Component.onCompleted: {
        rtcController.refreshDevices();
        applySystemNow();
    }

    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"
    }
    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            anchors.margins: 20
            spacing: 16
            width: app.width

            // Заголовок и настройки устройства
            Frame {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                background: Rectangle {
                    border.color: "#e0e0e0"
                    border.width: 1
                    color: "white"
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Label {
                        color: "#333"
                        font.bold: true
                        font.pixelSize: 16
                        text: "Настройки устройства"
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            Layout.preferredWidth: 140
                            text: "🔄 Обновить"

                            onClicked: rtcController.refreshDevices()
                        }
                        Label {
                            color: "#666"
                            text: "FT4222:"
                        }
                        ComboBox {
                            id: deviceSelector

                            Layout.fillWidth: true
                            model: rtcController.devices

                            onCurrentIndexChanged: rtcController.setSelectedIndex(currentIndex)
                        }
                        Label {
                            color: "#666"
                            text: "Адрес RTC:"
                        }
                        TextField {
                            id: addressField

                            placeholderText: "0x68"
                            text: "0x68"
                            width: 100

                            validator: RegularExpressionValidator {
                                regularExpression: /0x?[0-9a-fA-F]{1,2}/
                            }

                            onTextChanged: {
                                const parsed = parseInt(text, 16);
                                if (!isNaN(parsed)) {
                                    rtcController.setRtcAddress(parsed);
                                }
                            }
                        }
                    }
                }
            }

            // Текущее время на RTC
            Frame {
                Layout.fillWidth: true
                Layout.preferredHeight: 140

                background: Rectangle {
                    border.color: "#4CAF50"
                    border.width: 2
                    color: "white"
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            color: "#2E7D32"
                            font.bold: true
                            font.pixelSize: 18
                            text: "📅 Время на RTC"
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Label {
                            color: "#1B5E20"
                            font.bold: true
                            font.family: "monospace"
                            font.pixelSize: 20
                            padding: 8
                            text: rtcController.lastTimestamp

                            background: Rectangle {
                                color: "#E8F5E9"
                                radius: 4
                            }
                        }
                    }
                    RowLayout {
                        spacing: 12

                        Button {
                            Layout.preferredWidth: 160
                            text: "📖 Считать время"

                            onClicked: rtcController.readTime()
                        }
                        Button {
                            Layout.preferredWidth: 240
                            text: "💾 Записать системное (UTC)"

                            onClicked: {
                                applySystemNow();
                                rtcController.setToSystemTime();
                            }
                        }
                    }
                }
            }

            // Ручная установка времени
            Frame {
                Layout.fillWidth: true
                Layout.preferredHeight: 200

                background: Rectangle {
                    border.color: "#2196F3"
                    border.width: 2
                    color: "white"
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Label {
                        color: "#1565C0"
                        font.bold: true
                        font.pixelSize: 16
                        text: "⏰ Задать вручную (UTC)"
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columnSpacing: 12
                        columns: 6
                        rowSpacing: 10

                        Label {
                            Layout.alignment: Qt.AlignRight
                            color: "#666"
                            text: "Год:"
                        }
                        SpinBox {
                            id: yearBox

                            Layout.fillWidth: true
                            editable: true
                            from: 2000
                            to: 2099
                            value: 2024
                        }
                        Label {
                            Layout.alignment: Qt.AlignRight
                            color: "#666"
                            text: "Месяц:"
                        }
                        SpinBox {
                            id: monthBox

                            Layout.fillWidth: true
                            editable: true
                            from: 1
                            to: 12
                            value: 1
                        }
                        Label {
                            Layout.alignment: Qt.AlignRight
                            color: "#666"
                            text: "День:"
                        }
                        SpinBox {
                            id: dayBox

                            Layout.fillWidth: true
                            editable: true
                            from: 1
                            to: 31
                            value: 1
                        }
                        Label {
                            Layout.alignment: Qt.AlignRight
                            color: "#666"
                            text: "Час:"
                        }
                        SpinBox {
                            id: hourBox

                            Layout.fillWidth: true
                            editable: true
                            from: 0
                            to: 23
                            value: 0
                        }
                        Label {
                            Layout.alignment: Qt.AlignRight
                            color: "#666"
                            text: "Минута:"
                        }
                        SpinBox {
                            id: minuteBox

                            Layout.fillWidth: true
                            editable: true
                            from: 0
                            to: 59
                            value: 0
                        }
                        Label {
                            Layout.alignment: Qt.AlignRight
                            color: "#666"
                            text: "Секунда:"
                        }
                        SpinBox {
                            id: secondBox

                            Layout.fillWidth: true
                            editable: true
                            from: 0
                            to: 59
                            value: 0
                        }
                    }
                    RowLayout {
                        spacing: 12

                        Button {
                            Layout.preferredWidth: 180
                            text: "✅ Записать в RTC"

                            onClicked: rtcController.setDateTime(yearBox.value, monthBox.value, dayBox.value, hourBox.value, minuteBox.value, secondBox.value)
                        }
                        Button {
                            Layout.preferredWidth: 220
                            text: "🔄 Сбросить к системному"

                            onClicked: applySystemNow()
                        }
                    }
                }
            }

            // Control регистры PCF8523
            Frame {
                Layout.fillWidth: true
                Layout.preferredHeight: 500

                background: Rectangle {
                    border.color: "#FF9800"
                    border.width: 2
                    color: "white"
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            color: "#E65100"
                            font.bold: true
                            font.pixelSize: 16
                            text: "⚙️ Control регистры PCF8523"
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Button {
                            Layout.preferredWidth: 180
                            text: "📖 Прочитать регистры"

                            onClicked: rtcController.readControlRegisters()
                        }
                    }
                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true

                        Column {
                            spacing: 2
                            width: parent.width

                            // Заголовки таблицы
                            Row {
                                spacing: 4
                                width: parent.width

                                Label {
                                    color: "#E65100"
                                    font.bold: true
                                    padding: 6
                                    text: "Регистр"
                                    width: 120

                                    background: Rectangle {
                                        color: "#FFE0B2"
                                        radius: 3
                                    }
                                }
                                Label {
                                    color: "#E65100"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    padding: 6
                                    text: "Бит"
                                    width: 50

                                    background: Rectangle {
                                        color: "#FFE0B2"
                                        radius: 3
                                    }
                                }
                                Label {
                                    color: "#E65100"
                                    font.bold: true
                                    padding: 6
                                    text: "Название"
                                    width: 100

                                    background: Rectangle {
                                        color: "#FFE0B2"
                                        radius: 3
                                    }
                                }
                                Label {
                                    color: "#E65100"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    padding: 6
                                    text: "Значение"
                                    width: 70

                                    background: Rectangle {
                                        color: "#FFE0B2"
                                        radius: 3
                                    }
                                }
                                Label {
                                    color: "#E65100"
                                    font.bold: true
                                    padding: 6
                                    text: "Описание"
                                    width: parent.width - 340

                                    background: Rectangle {
                                        color: "#FFE0B2"
                                        radius: 3
                                    }
                                }
                            }

                            // Строки для Control_1 (0x00)
                            Repeater {
                                model: 8

                                delegate: Row {
                                    property int bitNum: 7 - index
                                    property bool bitValue: (rtcController.ctrl1Value >> bitNum) & 1

                                    spacing: 4
                                    width: parent.width

                                    Label {
                                        color: "#666"
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        padding: 4
                                        text: "CTRL1 (0x00)"
                                        width: 120

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.bold: true
                                        font.family: "monospace"
                                        horizontalAlignment: Text.AlignHCenter
                                        padding: 4
                                        text: bitNum.toString()
                                        width: 50

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        padding: 4
                                        text: rtcController.getBitName(0, bitNum)
                                        width: 100

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        color: bitValue ? "#4CAF50" : "#757575"
                                        font.bold: true
                                        font.family: "monospace"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        padding: 4
                                        text: bitValue ? "1" : "0"
                                        width: 70

                                        background: Rectangle {
                                            color: bitValue ? "#E8F5E9" : "#F5F5F5"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.pixelSize: 9
                                        padding: 4
                                        text: rtcController.getBitDescription(0, bitNum)
                                        width: parent.width - 340
                                        wrapMode: Text.WordWrap

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                }
                            }

                            // Строки для Control_2 (0x01)
                            Repeater {
                                model: 8

                                delegate: Row {
                                    property int bitNum: 7 - index
                                    property bool bitValue: (rtcController.ctrl2Value >> bitNum) & 1

                                    spacing: 4
                                    width: parent.width

                                    Label {
                                        color: "#666"
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        padding: 4
                                        text: "CTRL2 (0x01)"
                                        width: 120

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.bold: true
                                        font.family: "monospace"
                                        horizontalAlignment: Text.AlignHCenter
                                        padding: 4
                                        text: bitNum.toString()
                                        width: 50

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        padding: 4
                                        text: rtcController.getBitName(1, bitNum)
                                        width: 100

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        color: bitValue ? "#4CAF50" : "#757575"
                                        font.bold: true
                                        font.family: "monospace"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        padding: 4
                                        text: bitValue ? "1" : "0"
                                        width: 70

                                        background: Rectangle {
                                            color: bitValue ? "#E8F5E9" : "#F5F5F5"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.pixelSize: 9
                                        padding: 4
                                        text: rtcController.getBitDescription(1, bitNum)
                                        width: parent.width - 340
                                        wrapMode: Text.WordWrap

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                }
                            }

                            // Строки для Control_3 (0x02)
                            Repeater {
                                model: 8

                                delegate: Row {
                                    property int bitNum: 7 - index
                                    property bool bitValue: (rtcController.ctrl3Value >> bitNum) & 1

                                    spacing: 4
                                    width: parent.width

                                    Label {
                                        color: "#666"
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        padding: 4
                                        text: "CTRL3 (0x02)"
                                        width: 120

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.bold: true
                                        font.family: "monospace"
                                        horizontalAlignment: Text.AlignHCenter
                                        padding: 4
                                        text: bitNum.toString()
                                        width: 50

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        padding: 4
                                        text: rtcController.getBitName(2, bitNum)
                                        width: 100

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        color: bitValue ? "#4CAF50" : "#757575"
                                        font.bold: true
                                        font.family: "monospace"
                                        font.pixelSize: 12
                                        horizontalAlignment: Text.AlignHCenter
                                        padding: 4
                                        text: bitValue ? "1" : "0"
                                        width: 70

                                        background: Rectangle {
                                            color: bitValue ? "#E8F5E9" : "#F5F5F5"
                                            radius: 2
                                        }
                                    }
                                    Label {
                                        font.pixelSize: 9
                                        padding: 4
                                        text: rtcController.getBitDescription(2, bitNum)
                                        width: parent.width - 340
                                        wrapMode: Text.WordWrap

                                        background: Rectangle {
                                            color: "#FFF3E0"
                                            radius: 2
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Лог
            Frame {
                Layout.fillWidth: true
                Layout.minimumHeight: 150
                Layout.preferredHeight: 200

                background: Rectangle {
                    border.color: "#757575"
                    border.width: 1
                    color: "white"
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        color: "#424242"
                        font.bold: true
                        font.pixelSize: 14
                        text: "📋 Лог операций"
                    }
                    ScrollView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true

                        TextArea {
                            id: logArea

                            anchors.fill: parent
                            font.family: "monospace"
                            font.pixelSize: 11
                            readOnly: true
                            selectByMouse: true
                            text: rtcController.log
                            wrapMode: TextArea.WrapAnywhere

                            background: Rectangle {
                                color: "#FAFAFA"
                                radius: 4
                            }

                            onTextChanged: {
                                cursorPosition = length;
                            }
                        }
                    }
                }
            }
        }
    }
}
