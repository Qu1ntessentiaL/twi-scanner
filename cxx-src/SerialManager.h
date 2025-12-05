#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QTimer>

class SerialManager : public QObject {
Q_OBJECT

    Q_PROPERTY(QString portName READ getPortName WRITE setPortName NOTIFY portNameChanged)
    Q_PROPERTY(QString receivedData READ receivedData NOTIFY receivedDataChanged)
    Q_PROPERTY(QStringList ports READ ports NOTIFY portsChanged)
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged)

public:
    explicit SerialManager(QObject *parent = nullptr);

    QString getPortName() const { return m_portName; }

    void setPortName(const QString &name);

    QString receivedData() const { return m_receivedData; }

    QStringList ports() const { return m_ports; }

    Q_INVOKABLE void openPort();

    Q_INVOKABLE void closePort();

    Q_INVOKABLE void sendData(const QString &data);

    bool isOpen() const { return m_serial.isOpen(); }

signals:

    void portNameChanged();

    void receivedDataChanged();

    void portsChanged();

    void errorOccurred(const QString &msg);

    void isOpenChanged();

private slots:

    void handleReadyRead();

    void updatePorts();

private:
    QSerialPort m_serial;
    QString m_portName;
    QString m_receivedData;
    QStringList m_ports;
    QTimer m_timer;
};
