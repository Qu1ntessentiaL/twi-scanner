#pragma once

#include <QObject>
#include <QStringList>
#include <vector>

#include "ft4222.hpp"

/**
 * @brief Qt-обёртка для сканирования I2C через FT4222.
 *
 * Экспортируется в QML как context property `i2cScanner`.
 */
class I2CScanner : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString selectedSlave READ selectedSlave WRITE setSelectedSlave NOTIFY selectedSlaveChanged)

public:
    explicit I2CScanner(QObject* parent = nullptr);

    QStringList devices() const { return m_devicesList; }
    QString log() const { return m_log; }
    QString selectedSlave() const { return m_selectedSlave; }
    void setSelectedSlave(const QString& addr);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void scan(int startAddress = 0x03, int endAddress = 0x77);
    Q_INVOKABLE QByteArray readMemory(int slaveAddress, int offset, int length);
    Q_INVOKABLE bool writeMemory(int slaveAddress, int offset, const QByteArray& data);
    Q_INVOKABLE QString readRegistersHex(int slaveAddress, int start, int length);

signals:
    void devicesChanged();
    void logChanged();
    void selectedIndexChanged();
    void selectedSlaveChanged();
    void scanFinished();

private:
    void appendLog(const QString& msg);
    QString deviceLabel(const DeviceInfo& d) const;

    std::vector<DeviceInfo> m_devices;
    QStringList m_devicesList;
    QString m_log;
    QString m_selectedSlave = "0x3C";
    int m_selectedIndex = 0;
};
