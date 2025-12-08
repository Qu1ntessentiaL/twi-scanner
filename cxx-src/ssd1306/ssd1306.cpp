#include "ssd1306.h"

#include <cstdarg>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

using namespace std::chrono_literals;

ssd1306::ssd1306(std::shared_ptr<FTDevice> i2cDevice, uint8_t i2cAddr)
    : m_i2cDevice(std::move(i2cDevice))
      , m_i2cAddress(i2cAddr)
      , m_cursorX(0)
      , m_cursorY(0)
      , m_inverted(false)
      , m_initialized(false) {
    // Инициализируем буфер нулями (черный экран)
    std::fill(std::begin(m_buffer), std::end(m_buffer), 0x00);
}

ssd1306::ssd1306(FTDevice &i2cDevice, uint8_t i2cAddr)
    : m_i2cDevice(std::shared_ptr<FTDevice>(&i2cDevice, [](FTDevice *) {})) // Пустой deleter
      , m_i2cAddress(i2cAddr)
      , m_cursorX(0)
      , m_cursorY(0)
      , m_inverted(false)
      , m_initialized(false) {
    // Инициализируем буфер нулями (черный экран)
    std::fill(std::begin(m_buffer), std::end(m_buffer), 0x00);
}

bool ssd1306::writeCommand(uint8_t command) {
    try {
        // Для SSD1306: контрольный байт 0x00 (Co=0, D/C=0) для команд
        // Отправляем отдельно каждый байт команды
        std::vector<uint8_t> data = {0x00, command};
        m_i2cDevice->i2cMasterWrite(m_i2cAddress, data, 0x00);
        return true;
    }
    catch (const std::exception &e) {
        std::string errorMsg = "Failed to write command 0x";
        errorMsg += std::to_string(static_cast<int>(command));
        errorMsg += ": ";
        errorMsg += e.what();
        log(errorMsg);
        return false;
    }
}

bool ssd1306::writeCommands(const std::vector<uint8_t> &commands) {
    try {
        // Отправляем каждую команду отдельно
        for (uint8_t cmd : commands) {
            if (!writeCommand(cmd)) {
                return false;
            }
        }
        return true;
    }
    catch (const std::exception &e) {
        std::string errorMsg = "Failed to write commands: ";
        errorMsg += e.what();
        log(errorMsg);
        return false;
    }
}

bool ssd1306::writeData(const uint8_t *data, size_t size) {
    if (size == 0) return true;

    try {
        // Для SSD1306: контрольный байт 0x40 (Co=0, D/C=1) для данных
        // Но мы должны отправлять его только один раз в начале блока данных

        std::vector<uint8_t> buffer;
        buffer.reserve(size + 1);
        buffer.push_back(0x40); // Control byte for data (Co=0, D/C=1)

        // Копируем данные
        buffer.insert(buffer.end(), data, data + size);

        // Отправляем все данные одной транзакцией
        m_i2cDevice->i2cMasterWrite(m_i2cAddress, buffer, 0x00);
        return true;
    }
    catch (const std::exception &e) {
        std::string errorMsg = "Failed to write data: ";
        errorMsg += e.what();
        log(errorMsg);
        return false;
    }
}

void ssd1306::log(const std::string &message) const {
    if (m_logger) {
        m_logger(message);
    }
    else {
        std::cout << "[SSD1306] " << message << std::endl;
    }
}

void ssd1306::delay(uint32_t microseconds) const {
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

bool ssd1306::init() {
    if (m_initialized) {
        log("Display already initialized");
        return true;
    }

    if (!m_i2cDevice || !m_i2cDevice->isOpen()) {
        log("I2C device is not open");
        return false;
    }

    try {
        // Небольшая задержка для стабилизации питания
        std::this_thread::sleep_for(100ms);

        // Последовательность команд инициализации SSD1306
        // ОТПРАВЛЯЕМ КАЖДУЮ КОМАНДУ ОТДЕЛЬНО!

        // Выключаем дисплей перед настройкой
        writeCommand(0xAE); // Display off

        // Настройка адресации
        writeCommand(0x20); // Set Memory Addressing Mode
        writeCommand(0x00); // Horizontal addressing mode

        // Настройка отображения
        writeCommand(0x40); // Set display start line to 0

        // Настройка контраста
        writeCommand(0x81); // Set contrast control
        writeCommand(0xFF); // Контраст 255 (максимум)

        // Настройка сегментов
        writeCommand(0xA1); // Segment re-map (column 127 mapped to SEG0)

        // Настройка мультиплексирования
        writeCommand(0xA8); // Set multiplex ratio
        writeCommand(0x3F); // 64MUX for 128x64

        // Настройка COM сканирования
        writeCommand(0xC8); // COM output scan direction (remapped mode)

        // Настройка смещения дисплея
        writeCommand(0xD3); // Set display offset
        writeCommand(0x00); // No offset

        // Настройка тактовой частоты
        writeCommand(0xD5); // Set display clock divide ratio/oscillator
        writeCommand(0x80); // Default

        // Настройка предзаряда
        writeCommand(0xD9); // Set pre-charge period
        writeCommand(0xF1); // Phase1 = 15, Phase2 = 1

        // Настройка COM пинов
        writeCommand(0xDA); // Set COM pins hardware configuration
        writeCommand(0x12); // Alternative COM pin config, disable COM left/right remap

        // Настройка VCOMH
        writeCommand(0xDB); // Set VCOMH deselect level
        writeCommand(0x40); // ~0.77 x VCC

        // Включаем внутренний DC-DC преобразователь
        writeCommand(0x8D); // Charge pump setting
        writeCommand(0x14); // Enable charge pump

        // Нормальный режим (не инверсный)
        writeCommand(0xA6); // Set normal display

        // Весь дисплей включаем
        writeCommand(0xA4); // Entire display on (ignore RAM)

        // Включаем дисплей
        writeCommand(0xAF); // Display on

        // Очищаем буфер
        clear();
        updateScreen();

        // Сбрасываем позицию курсора
        m_cursorX = 0;
        m_cursorY = 0;

        m_initialized = true;
        log("Display initialized successfully");
        return true;
    }
    catch (const std::exception &e) {
        std::string errorMsg = "Initialization failed: ";
        errorMsg += e.what();
        log(errorMsg);
        return false;
    }
}

void ssd1306::displayOn() {
    if (!m_initialized) return;

    std::vector<uint8_t> commands = {
        0x8D, // Charge pump setting
        0x14, // Enable charge pump
        0xAF // Display ON
    };

    writeCommands(commands);
    log("Display turned ON");
}

void ssd1306::displayOff() {
    if (!m_initialized) return;

    std::vector<uint8_t> commands = {
        0xAE // Display OFF
    };

    writeCommands(commands);
    log("Display turned OFF");
}

void ssd1306::setContrast(uint8_t contrast) {
    if (!m_initialized) return;

    std::vector<uint8_t> commands = {
        0x81, // Set contrast control
        contrast
    };

    writeCommands(commands);
    log("Contrast set to " + std::to_string(static_cast<int>(contrast)));
}

void ssd1306::invertDisplay(bool invert) {
    if (!m_initialized) return;

    if (invert != m_inverted) {
        toggleInvert();
    }
}

void ssd1306::toggleInvert() {
    if (!m_initialized) return;

    // Инвертируем все пиксели в буфере
    for (auto &byte : m_buffer) {
        byte = ~byte;
    }

    // Отправляем команду инверсии на дисплей
    writeCommand(m_inverted ? 0xA6 : 0xA7);

    m_inverted = !m_inverted;
    log("Display invert " + std::string(m_inverted ? "enabled" : "disabled"));
}

void ssd1306::fill(COLOR color) {
    uint8_t pattern = (color == COLOR::WHITE) ? 0xFF : 0x00;
    std::fill(std::begin(m_buffer), std::end(m_buffer), pattern);
    log("Display filled with " + std::string(color == COLOR::WHITE ? "white" : "black"));
}

void ssd1306::clear() {
    fill(COLOR::BLACK);
}

void ssd1306::updateScreen() {
    if (!m_initialized) return;

    try {
        // SSD1306 использует постраничную адресацию
        for (uint8_t page = 0; page < 8; page++) {
            // Устанавливаем адрес страницы - используем отдельные команды
            writeCommand(0xB0 | page); // Set page address
            writeCommand(0x00); // Set lower column address
            writeCommand(0x10); // Set higher column address

            // Отправляем данные для текущей страницы (128 байт)
            uint8_t *pageData = &m_buffer[page * SSD1306_WIDTH];
            if (!writeData(pageData, SSD1306_WIDTH)) {
                std::string errorMsg = "Failed to write page data ";
                errorMsg += std::to_string(static_cast<int>(page));
                log(errorMsg);
                return;
            }
        }
    }
    catch (const std::exception &e) {
        std::string errorMsg = "Failed to update screen: ";
        errorMsg += e.what();
        log(errorMsg);
    }
}

void ssd1306::setCursor(uint16_t x, uint16_t y) {
    if (x >= SSD1306_WIDTH) x = SSD1306_WIDTH - 1;
    if (y >= SSD1306_HEIGHT) y = SSD1306_HEIGHT - 1;

    m_cursorX = x;
    m_cursorY = y;
}

void ssd1306::drawPixel(uint16_t x, uint16_t y, COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return; // Пиксель вне границ дисплея
    }

    // Определяем байт и бит в буфере
    uint16_t byteIndex = x + (y / 8) * SSD1306_WIDTH;
    uint8_t bitMask = 1 << (y % 8);

    // Устанавливаем или сбрасываем бит в зависимости от цвета
    if (color == COLOR::WHITE) {
        m_buffer[byteIndex] |= bitMask;
    }
    else {
        m_buffer[byteIndex] &= ~bitMask;
    }
}

char ssd1306::putChar(char ch, const FontDef_t *font, COLOR color) {
    if (!font || ch < 32 || ch > 126) {
        return 0; // Недопустимый символ или шрифт
    }

    // Проверяем, поместится ли символ на экране
    if (m_cursorX + font->FontWidth > SSD1306_WIDTH ||
        m_cursorY + font->FontHeight > SSD1306_HEIGHT) {
        return 0; // Недостаточно места
    }

    // Получаем данные символа из шрифта
    // Предполагаем, что шрифт хранит символы с 32 по 126
    uint32_t charIndex = (ch - 32) * font->FontHeight;

    // Рисуем символ по строкам (вертикальным байтам)
    for (uint32_t row = 0; row < font->FontHeight; row++) {
        // Получаем данные строки символа (шрифты обычно хранятся по столбцам)
        uint16_t fontData = font->data[charIndex + row];

        // Рисуем столбцы (биты в данных шрифта)
        for (uint32_t col = 0; col < font->FontWidth; col++) {
            // Проверяем бит в данных шрифта (старший бит соответствует левой стороне)
            bool pixelOn = (fontData << col) & 0x8000;

            // Рисуем пиксель с учетом цвета и инверсии
            COLOR pixelColor = pixelOn ? color : (color == COLOR::WHITE ? COLOR::BLACK : COLOR::WHITE);

            drawPixel(m_cursorX + col, m_cursorY + row, pixelColor);
        }
    }

    // Обновляем позицию курсора
    m_cursorX += font->FontWidth;

    return ch;
}

char ssd1306::putString(const char *str, const FontDef_t *font, COLOR color) {
    if (!str || !font) return 0;

    const char *originalStr = str;
    while (*str) {
        if (putChar(*str, font, color) != *str) {
            // Не удалось вывести символ
            return *str;
        }
        str++;
    }

    // Возвращаем 0 при успешном завершении
    return 0;
}

void ssd1306::printf(const FontDef_t *font, COLOR color, const char *format, ...) {
    if (!font) return;

    char buffer[128]; // Буфер для форматированной строки
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    putString(buffer, font, color);
}

void ssd1306::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color) {
    // Алгоритм Брезенхэма для рисования линии
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (dx > dy ? dx : -dy) / 2;
    int16_t e2;

    while (true) {
        drawPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void ssd1306::drawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color) {
    if (width == 0 || height == 0) return;

    // Проверяем границы
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    // Корректируем размеры если необходимо
    if (x + width > SSD1306_WIDTH) width = SSD1306_WIDTH - x;
    if (y + height > SSD1306_HEIGHT) height = SSD1306_HEIGHT - y;

    // Верхняя линия
    drawLine(x, y, x + width - 1, y, color);
    // Нижняя линия
    drawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
    // Левая линия
    drawLine(x, y, x, y + height - 1, color);
    // Правая линия
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

void ssd1306::drawFilledRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color) {
    if (width == 0 || height == 0) return;

    // Проверяем границы
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    // Корректируем размеры если необходимо
    if (x + width > SSD1306_WIDTH) width = SSD1306_WIDTH - x;
    if (y + height > SSD1306_HEIGHT) height = SSD1306_HEIGHT - y;

    // Заполняем прямоугольник горизонтальными линиями
    for (uint16_t row = 0; row < height; row++) {
        drawLine(x, y + row, x + width - 1, y + row, color);
    }
}

void ssd1306::drawCircle(int16_t x0, int16_t y0, int16_t radius, COLOR color) {
    if (radius <= 0) return;

    // Алгоритм Брезенхэма для рисования круга
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    // Рисуем начальные точки
    drawPixel(x0, y0 + radius, color);
    drawPixel(x0, y0 - radius, color);
    drawPixel(x0 + radius, y0, color);
    drawPixel(x0 - radius, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Рисуем все 8 октантов
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void ssd1306::drawFilledCircle(int16_t x0, int16_t y0, int16_t radius, COLOR color) {
    if (radius <= 0) return;

    // Алгоритм Брезенхэма с заполнением
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x = 0;
    int16_t y = radius;

    // Рисуем начальные линии
    drawLine(x0 - radius, y0, x0 + radius, y0, color);

    // Рисуем начальные точки
    drawPixel(x0, y0 + radius, color);
    drawPixel(x0, y0 - radius, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        // Заполняем горизонтальными линиями
        drawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
        drawLine(x0 - x, y0 - y, x0 + x, y0 - y, color);
        drawLine(x0 - y, y0 + x, x0 + y, y0 + x, color);
        drawLine(x0 - y, y0 - x, x0 + y, y0 - x, color);
    }
}

void ssd1306::drawTriangle(uint16_t x1, uint16_t y1,
                           uint16_t x2, uint16_t y2,
                           uint16_t x3, uint16_t y3,
                           COLOR color) {
    // Рисуем три линии, соединяющие вершины
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x3, y3, color);
    drawLine(x3, y3, x1, y1, color);
}

void ssd1306::drawFilledTriangle(uint16_t x1, uint16_t y1,
                                 uint16_t x2, uint16_t y2,
                                 uint16_t x3, uint16_t y3,
                                 COLOR color) {
    // Сортировка вершин по Y (y1 <= y2 <= y3)
    if (y1 > y2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    if (y1 > y3) {
        std::swap(x1, x3);
        std::swap(y1, y3);
    }
    if (y2 > y3) {
        std::swap(x2, x3);
        std::swap(y2, y3);
    }

    // Если треугольник плоский по горизонтали
    if (y2 == y3) {
        // Заполняем нижний плоский треугольник
        for (int16_t y = y1; y <= y2; y++) {
            int16_t xa = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            int16_t xb = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
            if (xa > xb) std::swap(xa, xb);
            drawLine(xa, y, xb, y, color);
        }
    }
    else if (y1 == y2) {
        // Заполняем верхний плоский треугольник
        for (int16_t y = y1; y <= y3; y++) {
            int16_t xa = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
            int16_t xb = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
            if (xa > xb) std::swap(xa, xb);
            drawLine(xa, y, xb, y, color);
        }
    }
    else {
        // Общий случай: треугольник делится на два плоских треугольника
        int16_t x4 = x1 + (x3 - x1) * (y2 - y1) / (y3 - y1);

        // Верхний плоский треугольник
        for (int16_t y = y1; y <= y2; y++) {
            int16_t xa = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            int16_t xb = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
            if (xa > xb) std::swap(xa, xb);
            drawLine(xa, y, xb, y, color);
        }

        // Нижний плоский треугольник
        for (int16_t y = y2; y <= y3; y++) {
            int16_t xa = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
            int16_t xb = x4 + (x3 - x4) * (y - y2) / (y3 - y2);
            if (xa > xb) std::swap(xa, xb);
            drawLine(xa, y, xb, y, color);
        }
    }
}
