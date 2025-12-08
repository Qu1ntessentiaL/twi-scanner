#pragma once

#include <cstring>
#include <functional>
#include <memory>
#include <vector>

#include "fonts.h"
#include "ft4222.hpp"

/* I2C address */
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR (0x3C)  ///< Стандартный I2C адрес SSD1306 (может быть 0x3C или 0x3D)
#endif

/* SSD1306 width in pixels */
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH 128  ///< Ширина дисплея в пикселях (128 для стандартного OLED)
#endif

/* SSD1306 height in pixels */
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT 64  ///< Высота дисплея в пикселях (64 для стандартного OLED)
#endif

/**
 * @brief Класс для работы с OLED дисплеем SSD1306 через интерфейс I2C
 *
 * Реализует высокоуровневый интерфейс для управления OLED дисплеем SSD1306
 * с разрешением 128x64 пикселей. Использует буферизацию изображения в памяти
 * с последующей передачей на дисплей через I2C.
 *
 * Особенности:
 * - Поддержка рисования геометрических фигур (линии, прямоугольники, круги)
 * - Вывод текста с использованием растровых шрифтов
 * - Инверсия цветов
 * - Обновление экрана по запросу (double buffering)
 */
class ssd1306 {
public:
    /**
     * @brief Цвета для дисплея SSD1306 (монохромный OLED)
     *
     * OLED дисплей SSD1306 монохромный: каждый пиксель может быть либо
     * включен (белый), либо выключен (черный).
     */
    enum class COLOR : uint8_t {
        BLACK = 0x00, ///< Черный цвет (пиксель выключен)
        WHITE = 0x01 ///< Белый цвет (пиксель включен)
    };

    /**
     * @brief Режимы адресации памяти дисплея
     */
    enum class ADDRESSING_MODE : uint8_t {
        HORIZONTAL = 0x00, ///< Горизонтальная адресация (по умолчанию)
        VERTICAL = 0x01, ///< Вертикальная адресация
        PAGE = 0x02 ///< Постраничная адресация (режим по умолчанию после сброса)
    };

    /**
     * @brief Направление сканирования строк (COM)
     */
    enum class COM_SCAN_DIRECTION : uint8_t {
        NORMAL = 0xC0, ///< Нормальное направление (сверху вниз)
        REVERSE = 0xC8 ///< Обратное направление (снизу вверх)
    };

    /**
     * @brief Направление отображения сегментов (COLUMN)
     */
    enum class SEGMENT_REMAP : uint8_t {
        NORMAL = 0xA0, ///< Нормальное направление (слева направо)
        REVERSE = 0xA1 ///< Обратное направление (справа налево)
    };

    /**
     * @brief Конструктор класса ssd1306
     * @param i2cDevice Указатель на инициализированный I2C мастер FT4222
     * @param i2cAddr I2C адрес дисплея SSD1306 (по умолчанию 0x3C)
     *
     * @warning Устройство FT4222 должно быть предварительно инициализировано
     * в режиме I2C Master с помощью initI2CMaster().
     */
    explicit ssd1306(std::shared_ptr<FTDevice> i2cDevice, uint8_t i2cAddr = SSD1306_I2C_ADDR);

    /**
     * @brief Конструктор класса ssd1306 (альтернативный)
     * @param i2cDevice Ссылка на инициализированный I2C мастер FT4222
     * @param i2cAddr I2C адрес дисплея SSD1306 (по умолчанию 0x3C)
     */
    explicit ssd1306(FTDevice &i2cDevice, uint8_t i2cAddr = SSD1306_I2C_ADDR);

    ~ssd1306() = default;

    // Удаляем конструкторы копирования
    ssd1306(const ssd1306 &) = delete;
    ssd1306 &operator=(const ssd1306 &) = delete;

    // Разрешаем перемещение
    ssd1306(ssd1306 &&) = default;
    ssd1306 &operator=(ssd1306 &&) = default;

    /**
     * @brief Инициализация дисплея SSD1306
     * @return true если инициализация успешна, false в случае ошибки
     *
     * Выполняет полную инициализацию дисплея:
     * 1. Отправляет последовательность команд инициализации
     * 2. Очищает буфер дисплея
     * 3. Включает дисплей
     *
     * @note После инициализации необходимо вызвать UpdateScreen() для отображения
     */
    bool init();

    /**
     * @brief Включение дисплея
     *
     * Активирует внутренний DC-DC преобразователь и включает OLED панель.
     */
    void displayOn();

    /**
     * @brief Выключение дисплея
     *
     * Отключает OLED панель для экономии энергии.
     */
    void displayOff();

    /**
     * @brief Установка контрастности дисплея
     * @param contrast Уровень контрастности (0-255)
     */
    void setContrast(uint8_t contrast);

    /**
     * @brief Инверсия цветов дисплея
     * @param invert true для инверсии, false для нормального режима
     *
     * В режиме инверсии черные пиксели становятся белыми и наоборот.
     */
    void invertDisplay(bool invert);

    /**
     * @brief Переключение инверсии цветов
     *
     * Меняет текущее состояние инверсии на противоположное.
     */
    void toggleInvert();

    /**
     * @brief Заполнение всего дисплея указанным цветом
     * @param color Цвет заполнения
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void fill(COLOR color);

    /**
     * @brief Очистка дисплея (заполнение черным)
     */
    void clear();

    /**
     * @brief Обновление физического дисплея из внутреннего буфера
     *
     * Передает содержимое буфера изображения на дисплей через I2C.
     * Должен вызываться после любых изменений в буфере.
     */
    void updateScreen();

    /**
     * @brief Установка позиции курсора для вывода текста
     * @param x Координата X (0-127)
     * @param y Координата Y (0-63)
     */
    void setCursor(uint16_t x, uint16_t y);

    /**
     * @brief Рисование отдельного пикселя
     * @param x Координата X
     * @param y Координата Y
     * @param color Цвет пикселя
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawPixel(uint16_t x, uint16_t y, COLOR color);

    /**
     * @brief Вывод одного символа
     * @param ch Символ для вывода
     * @param font Указатель на структуру шрифта
     * @param color Цвет текста
     * @return Выведенный символ или 0 при ошибке
     *
     * @note Автоматически обновляет позицию курсора
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    char putChar(char ch, const FontDef_t *font, COLOR color);

    /**
     * @brief Вывод строки
     * @param str Строка для вывода
     * @param font Указатель на структуру шрифта
     * @param color Цвет текста
     * @return Последний обработанный символ или 0 при успехе
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    char putString(const char *str, const FontDef_t *font, COLOR color);

    /**
     * @brief Форматированный вывод текста (аналог printf)
     * @param font Указатель на структуру шрифта
     * @param color Цвет текста
     * @param format Строка формата
     * @param ... Аргументы для форматирования
     */
    void printf(const FontDef_t *font, COLOR color, const char *format, ...);

    /**
     * @brief Рисование линии
     * @param x0 Начальная координата X
     * @param y0 Начальная координата Y
     * @param x1 Конечная координата X
     * @param y1 Конечная координата Y
     * @param color Цвет линии
     *
     * Использует алгоритм Брезенхэма для рисования линии.
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color);

    /**
     * @brief Рисование прямоугольника (только контур)
     * @param x Координата X верхнего левого угла
     * @param y Координата Y верхнего левого угла
     * @param width Ширина прямоугольника
     * @param height Высота прямоугольника
     * @param color Цвет прямоугольника
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color);

    /**
     * @brief Рисование заполненного прямоугольника
     * @param x Координата X верхнего левого угла
     * @param y Координата Y верхнего левого угла
     * @param width Ширина прямоугольника
     * @param height Высота прямоугольника
     * @param color Цвет прямоугольника
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawFilledRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color);

    /**
     * @brief Рисование круга (только контур)
     * @param x0 Координата X центра
     * @param y0 Координата Y центра
     * @param radius Радиус круга
     * @param color Цвет круга
     *
     * Использует алгоритм Брезенхэма для рисования круга.
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawCircle(int16_t x0, int16_t y0, int16_t radius, COLOR color);

    /**
     * @brief Рисование заполненного круга
     * @param x0 Координата X центра
     * @param y0 Координата Y центра
     * @param radius Радиус круга
     * @param color Цвет круга
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawFilledCircle(int16_t x0, int16_t y0, int16_t radius, COLOR color);

    /**
     * @brief Рисование треугольника (только контур)
     * @param x1 Координата X первой вершины
     * @param y1 Координата Y первой вершины
     * @param x2 Координата X второй вершины
     * @param y2 Координата Y второй вершины
     * @param x3 Координата X третьей вершины
     * @param y3 Координата Y третьей вершины
     * @param color Цвет треугольника
     *
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawTriangle(uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2,
                      uint16_t x3, uint16_t y3,
                      COLOR color);

    /**
     * @brief Рисование заполненного треугольника
     * @param x1 Координата X первой вершины
     * @param y1 Координата Y первой вершины
     * @param x2 Координата X второй вершины
     * @param y2 Координата Y второй вершины
     * @param x3 Координата X третьей вершины
     * @param y3 Координата Y третьей вершины
     * @param color Цвет треугольника
     *
     * Использует алгоритм заполнения по ребрам.
     * @note Для отображения изменений необходимо вызвать UpdateScreen()
     */
    void drawFilledTriangle(uint16_t x1, uint16_t y1,
                            uint16_t x2, uint16_t y2,
                            uint16_t x3, uint16_t y3,
                            COLOR color);

    /**
     * @brief Получение ширины дисплея
     * @return Ширина в пикселях
     */
    uint16_t getWidth() const { return SSD1306_WIDTH; }

    /**
     * @brief Получение высоты дисплея
     * @return Высота в пикселях
     */
    uint16_t getHeight() const { return SSD1306_HEIGHT; }

    /**
     * @brief Проверка инициализации дисплея
     * @return true если дисплей инициализирован
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Установка функции для логирования
     * @param logger Функция-логгер
     */
    void setLogger(std::function<void(const std::string &)> logger) {
        m_logger = std::move(logger);
    }

private:
    /**
     * @brief Отправка команды на дисплей
     * @param command Команда для отправки
     * @return true если команда отправлена успешно
     *
     * I2C транзакция: запись одного байта с контрольным байтом 0x00 (Co = 0, D/C = 0)
     */
    bool writeCommand(uint8_t command);

    /**
     * @brief Отправка нескольких команд на дисплей
     * @param commands Вектор команд
     * @return true если команды отправлены успешно
     */
    bool writeCommands(const std::vector<uint8_t> &commands);

    /**
     * @brief Отправка данных на дисплей
     * @param data Указатель на данные
     * @param size Размер данных в байтах
     * @return true если данные отправлены успешно
     *
     * I2C транзакция: запись данных с контрольным байтом 0x40 (Co = 0, D/C = 1)
     */
    bool writeData(const uint8_t *data, size_t size);

    /**
     * @brief Макрос для вычисления абсолютного значения
     */
    template<typename T>
    static T abs(T value) { return value < 0 ? -value : value; }

    /**
     * @brief Запись в лог
     * @param message Сообщение для логирования
     */
    void log(const std::string &message) const;

    /**
     * @brief Ожидание готовности дисплея (задержка)
     * @param microseconds Время ожидания в микросекундах
     *
     * Используется для соблюдения временных диаграмм инициализации.
     */
    void delay(uint32_t microseconds) const;

private:
    std::shared_ptr<FTDevice> m_i2cDevice; ///< Указатель на I2C устройство
    uint8_t m_i2cAddress; ///< I2C адрес дисплея
    uint8_t m_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8]; ///< Буфер дисплея (1 бит на пиксель)
    uint16_t m_cursorX; ///< Текущая позиция курсора X
    uint16_t m_cursorY; ///< Текущая позиция курсора Y
    bool m_inverted; ///< Флаг инверсии цветов
    bool m_initialized; ///< Флаг инициализации
    std::function<void(const std::string &)> m_logger; ///< Функция для логирования
};
