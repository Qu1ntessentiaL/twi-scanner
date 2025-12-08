#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "ft4222.hpp"
#include "ssd1306.h"

using namespace std::chrono_literals;

void logger(const std::string& message) {
    std::cout << "[TEST] " << message << std::endl;
}

int main() {
    logger("Тест SSD1306 с FT4222");

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
        auto ft4222Device = std::make_shared<FTDevice>(devices[0].index, logger);

        if (!ft4222Device->isOpen()) {
            logger("Не удалось открыть FT4222");
            return 1;
        }

        // 3. Инициализация I2C
        logger("Инициализация I2C Master...");
        ft4222Device->initI2CMaster(FTDevice::I2CSpeed::S400K);

        // 4. Простой тест I2C - сканирование шины
        logger("Сканирование I2C шины...");

        // 5. Инициализация дисплея
        logger("Инициализация SSD1306...");
        ssd1306 display(ft4222Device, 0x3C);
        display.setLogger(logger);

        // Упрощенный тест
        logger("Запуск упрощенного теста...");

        // Тест 1: Простая инициализация
        if (!display.init()) {
            logger("Ошибка инициализации дисплея!");

            // Попробуем альтернативный адрес
            logger("Пробуем адрес 0x3D...");
            ssd1306 display2(ft4222Device, 0x3D);
            display2.setLogger(logger);

            if (!display2.init()) {
                logger("Оба адреса не работают!");
                return 1;
            }

            // Продолжаем с display2
            display = std::move(display2);
        }

        logger("Дисплей инициализирован!");

        // Тест 2: Заполнение экрана
        logger("Тест: Заполнение белым...");
        display.fill(ssd1306::COLOR::WHITE);
        display.updateScreen();
        std::this_thread::sleep_for(2s);

        logger("Тест: Заполнение черным...");
        display.fill(ssd1306::COLOR::BLACK);
        display.updateScreen();
        std::this_thread::sleep_for(2s);

        // Тест 3: Рисование рамки
        logger("Тест: Рисование рамки...");
        display.drawRectangle(0, 0, 127, 63, ssd1306::COLOR::WHITE);
        display.updateScreen();
        std::this_thread::sleep_for(2s);

        // Тест 4: Перекрестие
        logger("Тест: Перекрестие...");
        display.clear();
        display.drawLine(0, 0, 127, 63, ssd1306::COLOR::WHITE);
        display.drawLine(127, 0, 0, 63, ssd1306::COLOR::WHITE);
        display.drawLine(63, 0, 63, 63, ssd1306::COLOR::WHITE);
        display.drawLine(0, 31, 127, 31, ssd1306::COLOR::WHITE);
        display.updateScreen();
        std::this_thread::sleep_for(2s);

        // Тест 5: Шахматная доска
        logger("Тест: Шахматная доска...");
        display.clear();
        for (int y = 0; y < 64; y += 8) {
            for (int x = 0; x < 128; x += 8) {
                if (((x / 8) + (y / 8)) % 2 == 0) {
                    display.drawFilledRectangle(x, y, 8, 8, ssd1306::COLOR::WHITE);
                }
            }
        }
        display.updateScreen();
        std::this_thread::sleep_for(3s);

        logger("Все тесты завершены!");

        // Выключаем дисплей
        display.displayOff();

    } catch (const std::exception& e) {
        logger("Ошибка: " + std::string(e.what()));
        return 1;
    }

    return 0;
}