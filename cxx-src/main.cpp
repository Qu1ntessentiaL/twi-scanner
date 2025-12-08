#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "ft4222.hpp"

void logger(const std::string& message) {
    std::cout << "[TEST] " << message << std::endl;
}

int main() {
    logger("Сканирование I2C с FT4222");

    try {
        // 1. Поиск устройств
        logger("Поиск FT4222 устройств...");
        auto devices = DeviceEnumerator::listDevices();

        if (devices.empty()) {
            logger("Устройства не найдены!");
            return 1;
        }

        logger("Найдено устройств: " + std::to_string(devices.size()));

        // 2. Инициализация FT4222
        logger("Инициализация FT4222...");
        const auto ft4222Device = std::make_shared<FTDevice>(devices[0].index, logger);

        if (!ft4222Device->isOpen()) {
            logger("Не удалось открыть FT4222");
            return 1;
        }

        // 3. Инициализация I2C
        logger("Инициализация I2C Master...");
        ft4222Device->initI2CMaster(FTDevice::I2CSpeed::S400K);

        // 4. Сканирование I2C шины
        logger("Сканирование I2C шины...");
        const auto foundAddresses = ft4222Device->scanI2CBus();

        if (foundAddresses.empty()) {
            logger("Устройства на шине не найдены");
        } else {
            std::ostringstream oss;
            oss << "Найдено устройств: " << foundAddresses.size() << " [";
            for (size_t i = 0; i < foundAddresses.size(); ++i) {
                if (i > 0) {
                    oss << ", ";
                }
                oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(foundAddresses[i]);
            }
            oss << "]";
            logger(oss.str());
        }

        // 5. Сбрасываем шину, чтобы отпустить линии после сканирования
        try {
            ft4222Device->i2cMasterResetBus();
            logger("I2C шина сброшена");
        } catch (const std::exception& resetErr) {
            logger("Не удалось сбросить шину: " + std::string(resetErr.what()));
        }

    } catch (const std::exception& e) {
        logger("Ошибка: " + std::string(e.what()));
        return 1;
    }

    return 0;
}