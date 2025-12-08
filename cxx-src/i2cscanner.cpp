#include "i2cscanner.hpp"

#include <iomanip>
#include <sstream>

I2CScanner::I2CScanner(QObject* parent) : QObject(parent) {
    refreshDevices();
}

void I2CScanner::setSelectedIndex(int idx) {
    if (idx == m_selectedIndex || idx < 0 || idx >= static_cast<int>(m_devices.size())) return;
    m_selectedIndex = idx;
    emit selectedIndexChanged();
}

void I2CScanner::refreshDevices() {
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

        if (m_devices.empty()) {
            appendLog("Устройства FT4222 не найдены");
        } else {
            appendLog(QString("Найдено устройств: %1").arg(m_devices.size()));
        }

        emit devicesChanged();
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка перечисления: %1").arg(e.what()));
    }
}

void I2CScanner::setSelectedSlave(const QString& addr) {
    if (addr == m_selectedSlave) return;
    m_selectedSlave = addr;
    emit selectedSlaveChanged();
}

void I2CScanner::scan(int startAddress, int endAddress) {
    if (m_devices.empty()) {
        appendLog("Нет устройств для сканирования");
        return;
    }

    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_devices.size())) {
        appendLog("Неверный индекс устройства");
        return;
    }

    if (startAddress < 0x00) startAddress = 0x00;
    if (endAddress > 0x7F) endAddress = 0x7F;
    if (startAddress > endAddress) std::swap(startAddress, endAddress);

    appendLog(QString("Сканирование адресов 0x%1–0x%2")
                  .arg(startAddress, 2, 16, QLatin1Char('0'))
                  .arg(endAddress, 2, 16, QLatin1Char('0')));

    const auto& devInfo = m_devices[static_cast<size_t>(m_selectedIndex)];

    // Локальный логгер FT4222, чтобы отправлять сообщения в UI
    auto logger = [this](const std::string& msg) {
        appendLog(QString::fromStdString(msg));
    };

    try {
        auto dev = std::make_shared<FTDevice>(devInfo.index, logger);
        dev->initI2CMaster(FTDevice::I2CSpeed::S400K);

        const auto addresses =
            dev->scanI2CBus(static_cast<uint8_t>(startAddress), static_cast<uint8_t>(endAddress));

        if (addresses.empty()) {
            appendLog("Устройства на шине не найдены");
        } else {
            QStringList found;
            for (uint8_t addr : addresses) {
                found << QString("0x%1").arg(addr, 2, 16, QLatin1Char('0'));
            }
            appendLog(QString("Найдено %1 адрес(ов): %2")
                          .arg(addresses.size())
                          .arg(found.join(", ")));
        }

        // Освободим линии после сканирования
        try {
            dev->i2cMasterResetBus();
            appendLog("I2C шина сброшена");
        } catch (const std::exception& resetErr) {
            appendLog(QString("Не удалось сбросить шину: %1").arg(resetErr.what()));
        }

        emit scanFinished();
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка сканирования: %1").arg(e.what()));
    } catch (...) {
        appendLog("Неизвестная ошибка сканирования");
    }
}

QByteArray I2CScanner::readMemory(int slaveAddress, int offset, int length) {
    if (m_devices.empty()) {
        appendLog("Нет устройств для чтения");
        return {};
    }
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_devices.size())) {
        appendLog("Неверный индекс устройства");
        return {};
    }
    if (length <= 0) {
        appendLog("Длина чтения должна быть > 0");
        return {};
    }

    const auto& devInfo = m_devices[static_cast<size_t>(m_selectedIndex)];

    try {
        auto dev = std::make_shared<FTDevice>(devInfo.index, nullptr);
        dev->initI2CMaster(FTDevice::I2CSpeed::S400K);

        // Пишем адрес регистра (START), затем повторный START для чтения и STOP в конце
        std::vector<uint8_t> reg = {static_cast<uint8_t>(offset & 0xFF)};
        dev->i2cMasterWrite(static_cast<uint8_t>(slaveAddress), reg, 0x02); // START, без STOP
        auto data = dev->i2cMasterRead(static_cast<uint8_t>(slaveAddress),
                                       static_cast<size_t>(length),
                                       0x07); // Repeated START + STOP

        QByteArray out;
        out.resize(static_cast<int>(data.size()));
        std::copy(data.begin(), data.end(), out.begin());

        std::ostringstream oss;
        oss << "READ 0x" << std::hex << slaveAddress << " @" << offset << " len=" << length;
        appendLog(QString::fromStdString(oss.str()));

        return out;
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка чтения: %1").arg(e.what()));
    }
    return {};
}

bool I2CScanner::writeMemory(int slaveAddress, int offset, const QByteArray& data) {
    if (m_devices.empty()) {
        appendLog("Нет устройств для записи");
        return false;
    }
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_devices.size())) {
        appendLog("Неверный индекс устройства");
        return false;
    }
    if (data.isEmpty()) {
        appendLog("Нет данных для записи");
        return false;
    }

    const auto& devInfo = m_devices[static_cast<size_t>(m_selectedIndex)];

    try {
        auto dev = std::make_shared<FTDevice>(devInfo.index, nullptr);
        dev->initI2CMaster(FTDevice::I2CSpeed::S400K);

        std::vector<uint8_t> payload;
        payload.reserve(data.size() + 1);
        payload.push_back(static_cast<uint8_t>(offset & 0xFF));
        payload.insert(payload.end(), data.begin(), data.end());

        dev->i2cMasterWrite(static_cast<uint8_t>(slaveAddress), payload, 0x06); // START+STOP

        std::ostringstream oss;
        oss << "WRITE 0x" << std::hex << slaveAddress << " @" << offset
            << " len=" << data.size();
        appendLog(QString::fromStdString(oss.str()));

        return true;
    } catch (const std::exception& e) {
        appendLog(QString("Ошибка записи: %1").arg(e.what()));
        return false;
    }
}

QString I2CScanner::readRegistersHex(int slaveAddress, int start, int length) {
    QStringList hex;
    for (int i = 0; i < length; ++i) {
        const QByteArray b = readMemory(slaveAddress, start + i, 1);
        if (!b.isEmpty()) {
            hex << QString("%1").arg(static_cast<uint8_t>(b.at(0)), 2, 16, QLatin1Char('0'));
        } else {
            hex << "??";
        }
    }
    return hex.join(' ');
}

void I2CScanner::appendLog(const QString& msg) {
    m_log.append(msg);
    m_log.append('\n');
    emit logChanged();
}

QString I2CScanner::deviceLabel(const DeviceInfo& d) const {
    return QString("#%1 %2 (%3)")
        .arg(d.index)
        .arg(QString::fromStdString(d.description))
        .arg(QString::fromStdString(d.serial));
}
