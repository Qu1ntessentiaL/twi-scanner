#include <QDebug>
#include "SerialManager.h"

SerialManager::SerialManager(QObject *parent) : QObject(parent) {
    connect(&m_serial, &QSerialPort::readyRead, this, &SerialManager::handleReadyRead);
    connect(&m_timer, &QTimer::timeout, this, &SerialManager::updatePorts);
    m_timer.start(1000);
    updatePorts();
}

void SerialManager::setPortName(const QString &name) {
    if (m_portName != name) {
        m_portName = name;
        emit portNameChanged();
    }
}

void SerialManager::updatePorts() {
    QStringList currentPorts;
    for (const QSerialPortInfo &info: QSerialPortInfo::availablePorts())
        currentPorts << info.portName();

    if (currentPorts != m_ports) {
        m_ports = currentPorts;
        emit portsChanged();
    }
}

void SerialManager::openPort() {
    if (m_serial.isOpen())
        m_serial.close();

    m_serial.setPortName(m_portName);
    m_serial.setBaudRate(QSerialPort::Baud115200);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred("Cannot open port: " + m_serial.errorString());
    }

    emit isOpenChanged();
}

void SerialManager::closePort() {
    if (m_serial.isOpen())
        m_serial.close();

    emit isOpenChanged();
}

void SerialManager::sendData(const QString &data) {
    if (m_serial.isOpen()) {
        m_serial.write(data.toUtf8());
    }
}

void SerialManager::handleReadyRead() {
    QByteArray data = m_serial.readAll();
    m_receivedData = QString::fromUtf8(data);
    emit receivedDataChanged();
}