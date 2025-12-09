#include "rtccontroller.hpp"

#include <QDate>
#include <QTime>
#include <QMap>

RTCController::RTCController(QObject* parent) : QObject(parent) {
    refreshDevices();
}

void RTCController::refreshDevices() {
    try {
        m_devices = DeviceEnumerator::listDevices();
        m_devicesList.clear();
        for (const auto& d : m_devices) {
            m_devicesList << deviceLabel(d);
        }

        if (m_selectedIndex >= static_cast<int>(m_devices.size())) {
            m_selectedIndex = m_devices.empty() ? 0 : 0;
            emit selectedIndexChanged();
        }

        emit devicesChanged();

        if (m_devices.empty()) {
            appendLog("FT4222 не найден");
        } else {
            appendLog(QString("Доступно FT4222: %1").arg(m_devices.size()));
        }
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка перечисления: %1").arg(e.what()));
    }
}

void RTCController::appendLog(const QString& msg) {
    m_log.append(msg);
    m_log.append('\n');
    emit logChanged();
}

QString RTCController::deviceLabel(const DeviceInfo& d) const {
    return QString("#%1 %2 (%3)")
        .arg(d.index)
        .arg(QString::fromStdString(d.description))
        .arg(QString::fromStdString(d.serial));
}

std::shared_ptr<FTDevice> RTCController::openCurrentDevice(bool enableLog) {
    if (m_devices.empty()) {
        appendLog("Нет устройств для открытия");
        return nullptr;
    }
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_devices.size())) {
        appendLog("Неверный индекс устройства");
        return nullptr;
    }

    const auto& devInfo = m_devices[static_cast<size_t>(m_selectedIndex)];
    auto logger = enableLog ? FTDevice::Logger(
                                  [this](const std::string& s) { appendLog(QString::fromStdString(s)); })
                            : FTDevice::Logger();

    try {
        auto dev = std::make_shared<FTDevice>(devInfo.index, logger);
        dev->initI2CMaster(FTDevice::I2CSpeed::S400K);
        return dev;
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка открытия: %1").arg(e.what()));
    }
    return nullptr;
}

uint8_t RTCController::toBcd(int value) {
    const int v = value % 100;
    return static_cast<uint8_t>(((v / 10) << 4) | (v % 10));
}

int RTCController::fromBcd(uint8_t value) {
    return static_cast<int>((value >> 4) * 10 + (value & 0x0F));
}

void RTCController::readTime() {
    auto dev = openCurrentDevice(true);
    if (!dev) return;

    const uint8_t addr = static_cast<uint8_t>(m_rtcAddress & 0x7F);
    try {
        // PCF8523: регистры времени начинаются с 0x03
        std::vector<uint8_t> reg = {0x03};
        dev->i2cMasterWrite(addr, reg, 0x02);          // START, без STOP
        auto data = dev->i2cMasterRead(addr, 7, 0x07); // Repeated START + STOP
        if (data.size() < 7) {
            appendLog("Недостаточно данных от RTC");
            return;
        }

        // PCF8523 структура регистров (все в BCD):
        // data[0] = 0x03: Seconds (бит 7 = OS - oscillator stop)
        // data[1] = 0x04: Minutes
        // data[2] = 0x05: Hours (24ч формат, бит 6 = 0)
        // data[3] = 0x06: Days
        // data[4] = 0x07: Weekdays (0=Sunday, 1=Monday, ..., 6=Saturday)
        // data[5] = 0x08: Months
        // data[6] = 0x09: Years

        const int seconds = fromBcd(data[0] & 0x7F);  // игнорируем бит OS
        const int minutes = fromBcd(data[1] & 0x7F);
        const int hours = fromBcd(data[2] & 0x3F);    // 24ч формат, бит 6 = 0
        const int day = fromBcd(data[3] & 0x3F);
        // data[4] - weekday, пропускаем
        const int month = fromBcd(data[5] & 0x1F);
        const int year = 2000 + fromBcd(data[6]);

        QDate date(year, month, day);
        QTime time(hours, minutes, seconds);
        if (!date.isValid() || !time.isValid()) {
            appendLog("RTC вернул некорректные дату/время");
            return;
        }

        const auto dt = QDateTime(date, time, Qt::UTC);
        m_lastTimestamp = dt.toString("yyyy-MM-dd hh:mm:ss 'UTC'");
        emit timeChanged();

        appendLog(QString("RTC -> %1").arg(m_lastTimestamp));
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка чтения: %1").arg(e.what()));
    }
}

bool RTCController::setDateTime(int year, int month, int day, int hour, int minute, int second) {
    QDate date(year, month, day);
    QTime time(hour, minute, second);
    if (!date.isValid() || !time.isValid()) {
        appendLog("Неверные дата/время");
        return false;
    }

    auto dev = openCurrentDevice(true);
    if (!dev) return false;

    // PCF8523: weekday 0=Sunday, 1=Monday, ..., 6=Saturday
    // Qt: dayOfWeek() возвращает 1=Monday, 2=Tuesday, ..., 7=Sunday
    int qtWeekday = date.dayOfWeek(); // 1..7
    int pcfWeekday = (qtWeekday == 7) ? 0 : qtWeekday; // конвертируем: 7->0, 1->1, 2->2, ...

    const uint8_t addr = static_cast<uint8_t>(m_rtcAddress & 0x7F);

    try {
        // PCF8523: запись начинается с регистра 0x03
        std::vector<uint8_t> payload;
        payload.reserve(8);
        payload.push_back(0x03); // стартовый регистр для PCF8523
        payload.push_back(toBcd(second) & 0x7F);          // 0x03: Seconds (бит 7 = OS, оставляем 0)
        payload.push_back(toBcd(minute) & 0x7F);          // 0x04: Minutes
        payload.push_back(toBcd(hour) & 0x3F);            // 0x05: Hours (24ч формат, бит 6 = 0)
        payload.push_back(toBcd(day) & 0x3F);              // 0x06: Days
        payload.push_back(static_cast<uint8_t>(pcfWeekday) & 0x07); // 0x07: Weekdays (0-6)
        payload.push_back(toBcd(month) & 0x1F);           // 0x08: Months
        payload.push_back(toBcd(date.year() % 100) & 0xFF); // 0x09: Years (00..99)

        dev->i2cMasterWrite(addr, payload, 0x06); // START + STOP

        const auto dt = QDateTime(date, time, Qt::UTC);
        m_lastTimestamp = dt.toString("yyyy-MM-dd hh:mm:ss 'UTC'");
        emit timeChanged();
        appendLog(QString("RTC <- %1").arg(m_lastTimestamp));
        return true;
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка записи: %1").arg(e.what()));
        return false;
    }
}

bool RTCController::setToSystemTime() {
    const auto now = QDateTime::currentDateTimeUtc();
    return setDateTime(now.date().year(),
                       now.date().month(),
                       now.date().day(),
                       now.time().hour(),
                       now.time().minute(),
                       now.time().second());
}

void RTCController::setSelectedIndex(int idx) {
    if (idx == m_selectedIndex) return;
    if (idx < 0 || idx >= static_cast<int>(m_devices.size())) return;
    m_selectedIndex = idx;
    emit selectedIndexChanged();
}

void RTCController::setRtcAddress(int addr) {
    if (addr == m_rtcAddress) return;
    if (addr < 0x00 || addr > 0x7F) return;
    m_rtcAddress = addr;
    emit rtcAddressChanged();
}

void RTCController::readControlRegisters() {
    auto dev = openCurrentDevice(true);
    if (!dev) return;

    const uint8_t addr = static_cast<uint8_t>(m_rtcAddress & 0x7F);
    try {
        // Читаем control регистры 0x00, 0x01, 0x02
        std::vector<uint8_t> reg = {0x00};
        dev->i2cMasterWrite(addr, reg, 0x02);          // START, без STOP
        auto data = dev->i2cMasterRead(addr, 3, 0x07); // Repeated START + STOP
        if (data.size() < 3) {
            appendLog("Недостаточно данных control регистров");
            return;
        }

        m_ctrl1 = data[0];
        m_ctrl2 = data[1];
        m_ctrl3 = data[2];

        // Форматируем информацию о регистрах
        QStringList info;

        // Control_1 (0x00)
        QMap<int, QString> ctrl1Bits;
        ctrl1Bits[7] = "EXT_TEST (External test mode)";
        ctrl1Bits[6] = "STOP (Stop bit)";
        ctrl1Bits[5] = "TEST (Test mode)";
        ctrl1Bits[4] = "12/24 (12/24 hour mode: 0=24h, 1=12h)";
        ctrl1Bits[3] = "SIE (Second interrupt enable)";
        ctrl1Bits[2] = "AIE (Alarm interrupt enable)";
        ctrl1Bits[1] = "CIE (Countdown timer interrupt enable)";
        ctrl1Bits[0] = "CAP_SEL (Capacitor selection: 0=7pF, 1=12.5pF)";
        info << formatControlRegister(m_ctrl1, 0x00, "Control_1 (CTRL1)", ctrl1Bits);

        // Control_2 (0x01)
        QMap<int, QString> ctrl2Bits;
        ctrl2Bits[7] = "TIE (Timer interrupt enable)";
        ctrl2Bits[6] = "AIE (Alarm interrupt enable)";
        ctrl2Bits[5] = "TAF (Timer alarm flag)";
        ctrl2Bits[4] = "AAF (Alarm flag)";
        ctrl2Bits[3] = "TIF (Timer interrupt flag)";
        ctrl2Bits[2] = "AIF (Alarm interrupt flag)";
        ctrl2Bits[1] = "Reserved";
        ctrl2Bits[0] = "Reserved";
        info << formatControlRegister(m_ctrl2, 0x01, "Control_2 (CTRL2)", ctrl2Bits);

        // Control_3 (0x02)
        QMap<int, QString> ctrl3Bits;
        ctrl3Bits[7] = "BLF (Battery low flag)";
        ctrl3Bits[6] = "BSF (Battery switch flag)";
        ctrl3Bits[5] = "BLIE (Battery low interrupt enable)";
        ctrl3Bits[4] = "BSIE (Battery switch interrupt enable)";
        ctrl3Bits[3] = "PWRMNG[3] (Power management bit 3)";
        ctrl3Bits[2] = "PWRMNG[2] (Power management bit 2)";
        ctrl3Bits[1] = "PWRMNG[1] (Power management bit 1)";
        ctrl3Bits[0] = "PWRMNG[0] (Power management bit 0)";
        info << formatControlRegister(m_ctrl3, 0x02, "Control_3 (CTRL3)", ctrl3Bits);

        // Дополнительная информация о PWRMNG
        uint8_t pwrmng = m_ctrl3 & 0x0F;
        QString pwrmngDesc;
        switch (pwrmng) {
            case 0x00: pwrmngDesc = "Normal mode"; break;
            case 0x01: pwrmngDesc = "Power-down mode"; break;
            case 0x02: pwrmngDesc = "Power-save mode"; break;
            default: pwrmngDesc = QString("Reserved/Unknown (0x%1)").arg(pwrmng, 2, 16, QLatin1Char('0')); break;
        }
        info << QString("  PWRMNG[3:0] = 0x%1: %2").arg(pwrmng, 2, 16, QLatin1Char('0')).arg(pwrmngDesc);

        m_controlRegsInfo = info.join("\n");
        emit controlRegsChanged();

        appendLog("Control регистры прочитаны");
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка чтения control регистров: %1").arg(e.what()));
    }
}

QString RTCController::formatControlRegister(uint8_t reg, int addr, const QString& name, const QMap<int, QString>& bitDescriptions) const {
    QStringList lines;
    lines << QString("%1 (0x%2): 0x%3 (0b%4)")
             .arg(name)
             .arg(addr, 2, 16, QLatin1Char('0'))
             .arg(reg, 2, 16, QLatin1Char('0'))
             .arg(reg, 8, 2, QLatin1Char('0'));

    for (int bit = 7; bit >= 0; --bit) {
        bool bitValue = (reg >> bit) & 1;
        QString bitName = bitDescriptions.value(bit, QString("Bit %1").arg(bit));
        lines << QString("  Bit %1: %2 = %3")
                 .arg(bit)
                 .arg(bitName)
                 .arg(bitValue ? "1" : "0");
    }

    return lines.join("\n");
}

QString RTCController::getBitName(int regNum, int bitNum) const {
    static QMap<QPair<int, int>, QString> bitNames;
    
    if (bitNames.isEmpty()) {
        // Control_1 (0x00)
        bitNames[QPair<int, int>(0, 7)] = "EXT_TEST";
        bitNames[QPair<int, int>(0, 6)] = "STOP";
        bitNames[QPair<int, int>(0, 5)] = "TEST";
        bitNames[QPair<int, int>(0, 4)] = "12/24";
        bitNames[QPair<int, int>(0, 3)] = "SIE";
        bitNames[QPair<int, int>(0, 2)] = "AIE";
        bitNames[QPair<int, int>(0, 1)] = "CIE";
        bitNames[QPair<int, int>(0, 0)] = "CAP_SEL";
        
        // Control_2 (0x01)
        bitNames[QPair<int, int>(1, 7)] = "TIE";
        bitNames[QPair<int, int>(1, 6)] = "AIE";
        bitNames[QPair<int, int>(1, 5)] = "TAF";
        bitNames[QPair<int, int>(1, 4)] = "AAF";
        bitNames[QPair<int, int>(1, 3)] = "TIF";
        bitNames[QPair<int, int>(1, 2)] = "AIF";
        bitNames[QPair<int, int>(1, 1)] = "Reserved";
        bitNames[QPair<int, int>(1, 0)] = "Reserved";
        
        // Control_3 (0x02)
        bitNames[QPair<int, int>(2, 7)] = "BLF";
        bitNames[QPair<int, int>(2, 6)] = "BSF";
        bitNames[QPair<int, int>(2, 5)] = "BLIE";
        bitNames[QPair<int, int>(2, 4)] = "BSIE";
        bitNames[QPair<int, int>(2, 3)] = "PWRMNG[3]";
        bitNames[QPair<int, int>(2, 2)] = "PWRMNG[2]";
        bitNames[QPair<int, int>(2, 1)] = "PWRMNG[1]";
        bitNames[QPair<int, int>(2, 0)] = "PWRMNG[0]";
    }
    
    return bitNames.value(QPair<int, int>(regNum, bitNum), QString("Bit %1").arg(bitNum));
}

QString RTCController::getBitDescription(int regNum, int bitNum) const {
    static QMap<QPair<int, int>, QString> bitDescs;
    
    if (bitDescs.isEmpty()) {
        // Control_1 (0x00)
        bitDescs[QPair<int, int>(0, 7)] = "External test mode";
        bitDescs[QPair<int, int>(0, 6)] = "Stop bit (0=run, 1=stop)";
        bitDescs[QPair<int, int>(0, 5)] = "Test mode";
        bitDescs[QPair<int, int>(0, 4)] = "12/24 hour mode (0=24h, 1=12h)";
        bitDescs[QPair<int, int>(0, 3)] = "Second interrupt enable";
        bitDescs[QPair<int, int>(0, 2)] = "Alarm interrupt enable";
        bitDescs[QPair<int, int>(0, 1)] = "Countdown timer interrupt enable";
        bitDescs[QPair<int, int>(0, 0)] = "Capacitor selection (0=7pF, 1=12.5pF)";
        
        // Control_2 (0x01)
        bitDescs[QPair<int, int>(1, 7)] = "Timer interrupt enable";
        bitDescs[QPair<int, int>(1, 6)] = "Alarm interrupt enable";
        bitDescs[QPair<int, int>(1, 5)] = "Timer alarm flag";
        bitDescs[QPair<int, int>(1, 4)] = "Alarm flag";
        bitDescs[QPair<int, int>(1, 3)] = "Timer interrupt flag";
        bitDescs[QPair<int, int>(1, 2)] = "Alarm interrupt flag";
        bitDescs[QPair<int, int>(1, 1)] = "Reserved";
        bitDescs[QPair<int, int>(1, 0)] = "Reserved";
        
        // Control_3 (0x02)
        bitDescs[QPair<int, int>(2, 7)] = "Battery low flag";
        bitDescs[QPair<int, int>(2, 6)] = "Battery switch flag";
        bitDescs[QPair<int, int>(2, 5)] = "Battery low interrupt enable";
        bitDescs[QPair<int, int>(2, 4)] = "Battery switch interrupt enable";
        bitDescs[QPair<int, int>(2, 3)] = "Power management bit 3";
        bitDescs[QPair<int, int>(2, 2)] = "Power management bit 2";
        bitDescs[QPair<int, int>(2, 1)] = "Power management bit 1";
        bitDescs[QPair<int, int>(2, 0)] = "Power management bit 0";
    }
    
    return bitDescs.value(QPair<int, int>(regNum, bitNum), "");
}
