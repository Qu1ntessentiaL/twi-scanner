#pragma once

#include <QObject>
#include <QDateTime>
#include <QStringList>
#include <QMap>
#include <QPair>
#include <vector>
#include <memory>

#include "ft4222.hpp"

/**
 * @brief Контроллер работы с I2C RTC (PCF8523).
 *
 * PCF8523: регистры времени начинаются с адреса 0x03, все значения в формате BCD.
 * Экспортируется в QML как context property `rtcController`.
 */
class RTCController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(int rtcAddress READ rtcAddress WRITE setRtcAddress NOTIFY rtcAddressChanged)
    Q_PROPERTY(QString lastTimestamp READ lastTimestamp NOTIFY timeChanged)
    Q_PROPERTY(QString controlRegsInfo READ controlRegsInfo NOTIFY controlRegsChanged)
    Q_PROPERTY(int ctrl1Value READ ctrl1Value NOTIFY controlRegsChanged)
    Q_PROPERTY(int ctrl2Value READ ctrl2Value NOTIFY controlRegsChanged)
    Q_PROPERTY(int ctrl3Value READ ctrl3Value NOTIFY controlRegsChanged)

public:
    explicit RTCController(QObject* parent = nullptr);

    QStringList devices() const { return m_devicesList; }
    int selectedIndex() const { return m_selectedIndex; }
    QString log() const { return m_log; }
    int rtcAddress() const { return m_rtcAddress; }
    QString lastTimestamp() const { return m_lastTimestamp; }

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void setSelectedIndex(int idx);
    Q_INVOKABLE void setRtcAddress(int addr);
    Q_INVOKABLE void readTime();
    Q_INVOKABLE bool setDateTime(int year, int month, int day, int hour, int minute, int second);
    Q_INVOKABLE bool setToSystemTime();
    Q_INVOKABLE void readControlRegisters();
    QString controlRegsInfo() const { return m_controlRegsInfo; }
    int ctrl1Value() const { return m_ctrl1; }
    int ctrl2Value() const { return m_ctrl2; }
    int ctrl3Value() const { return m_ctrl3; }
    Q_INVOKABLE QString getBitName(int regNum, int bitNum) const;
    Q_INVOKABLE QString getBitDescription(int regNum, int bitNum) const;

signals:
    void devicesChanged();
    void selectedIndexChanged();
    void logChanged();
    void rtcAddressChanged();
    void timeChanged();
    void controlRegsChanged();

private:
    void appendLog(const QString& msg);
    QString deviceLabel(const DeviceInfo& d) const;
    std::shared_ptr<FTDevice> openCurrentDevice(bool enableLog);

    static uint8_t toBcd(int value);
    static int fromBcd(uint8_t value);

    QString formatControlRegister(uint8_t reg, int addr, const QString& name, const QMap<int, QString>& bitDescriptions) const;

    std::vector<DeviceInfo> m_devices;
    QStringList m_devicesList;
    int m_selectedIndex = 0;
    int m_rtcAddress = 0x68;           // типовой адрес DS1307/PCF8523
    QString m_log;
    QString m_lastTimestamp = "--";
    QString m_controlRegsInfo = "--";
    uint8_t m_ctrl1 = 0;
    uint8_t m_ctrl2 = 0;
    uint8_t m_ctrl3 = 0;
};
