#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// Включаем реальные заголовки FTDI
#include "ftd2xx.h"      // Базовые функции FTDI D2XX
#include "libft4222.h"   // Функции специфичные для FT4222

/**
 * @brief Структура с информацией об обнаруженном FTDI-устройстве
 *
 * @note  Содержит основные идентификационные данные устройства, полученные
 *        при перечислении устройств через DeviceEnumerator.
 */
struct DeviceInfo {
    std::string serial;      ///< Серийный номер устройства
    std::string description; ///< Текстовое описание устройства
    uint32_t index;          ///< Индекс устройства в списке FTDI
    uint32_t locationId;     ///< Идентификатор местоположения устройства (Location ID)
    uint32_t flags;          ///< Флаги состояния устройства
};

/**
 * @brief Исключение для ошибок FTDI с сохранением кода статуса
 *
 * @note  Наследует от std::runtime_error и добавляет код ошибки FT_STATUS
 *        для детализации причины сбоя при работе с FTDI API.
 */
class FtException final : public std::runtime_error {
public:
    /**
     * @brief Конструктор исключения с сообщением и кодом ошибки
     * @param msg Текстовое описание ошибки
     * @param ftStatus Код ошибки FT_STATUS (по умолчанию FT_OK)
     */
    explicit FtException(const std::string &msg, const FT_STATUS ftStatus = FT_OK)
        : std::runtime_error(msg + " (FT_STATUS=" + std::to_string(ftStatus) + ")"),
          status(ftStatus) {}

    FT_STATUS status; ///< Код ошибки FT_STATUS
};

/**
 * @brief Класс для поиска и перечисления FTDI-устройств в системе
 *
 * @note  Предоставляет статический метод для получения списка всех подключенных
 *        FTDI-устройств с фильтрацией по типу (только FT4222).
 */
class DeviceEnumerator {
public:
    /**
     * @brief Получить список всех FT4222-устройств в системе
     * @return Вектор структур DeviceInfo с информацией о каждом устройстве
     * @throw FtException При ошибке получения списка устройств
     */
    static std::vector<DeviceInfo> listDevices();
};

/**
 * @brief Основной класс для работы с FT4222
 *
 * @note  Реализует высокоуровневый интерфейс для работы с чипом FT4222H,
 *        поддерживающим режимы I2C Master, SPI Master и GPIO.
 *        Использует Pimpl идиому для сокрытия деталей реализации.
 */
class FTDevice {
public:
    /// Тип функции-логгера для вывода отладочной информации
    using Logger = std::function<void(const std::string &)>;

    /**
     * @brief Режимы работы FT4222
     *
     * @note  Определяет текущий активный режим работы устройства.
     *        Устройство может работать только в одном режиме одновременно.
     */
    enum class Mode {
        Unknown, ///< Режим не определен или устройство не инициализировано
        SPI_Master, ///< Режим SPI ведущего (master)
        SPI_Slave, ///< Режим SPI ведомого (slave) - не поддерживается в данной реализации
        I2C_Master, ///< Режим I2C ведущего (master)
        I2C_Slave, ///< Режим I2C ведомого (slave) - не поддерживается в данной реализации
        GPIO ///< Режим общего назначения ввода/вывода
    };

    /**
     * @brief Режимы работы GPIO выводов
     *
     * @note  Определяет направление каждого GPIO вывода при инициализации.
     */
    enum class GPIOMode {
        Input, ///< Вывод работает на вход
        Output ///< Вывод работает на выход
    };

    /**
     * @brief Скорости шины I2C в килобитах в секунду
     *
     * @note  Стандартные скорости I2C шины. Реальная скорость может отличаться
     *        в зависимости от системной частоты FT4222.
     */
    enum class I2CSpeed {
        S100K = 100, ///< Стандартный режим 100 кбит/с
        S400K = 400, ///< Быстрый режим 400 кбит/с
        S1M = 1000   ///< Высокоскоростной режим 1000 кбит/с (1 Мбит/с)
    };

    /**
     * @brief Делители частоты для SPI
     *
     * @note  Определяют коэффициент деления системной частоты для генерации
     *        тактовой частоты SPI. Более высокие значения = более низкая скорость.
     */
    enum class SPIClockDivider {
        DIV_2 = CLK_DIV_2,     ///< Деление на 2
        DIV_4 = CLK_DIV_4,     ///< Деление на 4
        DIV_8 = CLK_DIV_8,     ///< Деление на 8
        DIV_16 = CLK_DIV_16,   ///< Деление на 16
        DIV_32 = CLK_DIV_32,   ///< Деление на 32
        DIV_64 = CLK_DIV_64,   ///< Деление на 64
        DIV_128 = CLK_DIV_128, ///< Деление на 128
        DIV_256 = CLK_DIV_256, ///< Деление на 256
        DIV_512 = CLK_DIV_512  ///< Деление на 512
    };

    // Удаляем конструкторы копирования
    FTDevice(const FTDevice &) = delete;
    FTDevice &operator=(const FTDevice &) = delete;

    // Конструкторы перемещения
    FTDevice(FTDevice &&) noexcept;
    FTDevice &operator=(FTDevice &&) noexcept;

    /**
     * @brief Конструктор для открытия устройства по индексу
     * @param index Индекс устройства в списке (UINT32_MAX для отложенного открытия)
     * @param logger Функция для логирования (nullptr для отключения)
     */
    FTDevice(uint32_t index = UINT32_MAX, Logger logger = nullptr);

    /**
     * @brief Конструктор для открытия устройства по серийному номеру
     * @param serialNumber Серийный номер устройства
     * @param logger Функция для логирования (nullptr для отключения)
     */
    FTDevice(const std::string &serialNumber, Logger logger = nullptr);

    ~FTDevice();

    // Основные операции с устройством

    /**
     * @brief Открыть устройство по индексу
     * @param index Индекс устройства в списке FTDI
     * @throw std::runtime_error Если устройство уже открыто
     * @throw FtException При ошибке открытия устройства
     */
    void open(uint32_t index);

    /**
     * @brief Открыть устройство по серийному номеру
     * @param serialNumber Серийный номер устройства
     * @throw std::runtime_error Если устройство уже открыто
     * @throw FtException При ошибке открытия устройства
     */
    void openBySerial(const std::string &serialNumber);

    /**
     * @brief Закрыть устройство и освободить ресурсы
     *
     * @note  Гарантированно не бросает исключений (noexcept).
     */
    void close() noexcept;

    /**
     * @brief Проверить, открыто ли устройство
     * @return true устройство открыто и готово к работе
     */
    bool isOpen() const noexcept;

    // Общие операции чтения/записи

    /**
     * @brief Прочитать данные из устройства через интерфейс D2XX
     * @param bytesToRead Количество байт для чтения
     * @param timeoutMs Таймаут операции в миллисекундах
     * @return Вектор прочитанных байт (может быть меньше запрошенного)
     * @throw std::runtime_error Если устройство не открыто
     * @throw FtException При ошибке чтения
     */
    std::vector<uint8_t> read(size_t bytesToRead, unsigned int timeoutMs = 1000);

    /**
     * @brief Записать данные в устройство через интерфейс D2XX
     * @param data Данные для записи
     * @param timeoutMs Таймаут операции в миллисекундах
     * @throw std::runtime_error Если устройство не открыто
     * @throw FtException При ошибке записи
     */
    void write(const std::vector<uint8_t> &data, unsigned int timeoutMs = 1000);

    // Функции I2C Master режима

    /**
     * @brief Инициализировать устройство в режиме I2C Master
     * @param speed Скорость шины I2C
     * @throw std::runtime_error Если устройство не открыто
     * @throw std::runtime_error При ошибке инициализации
     */
    void initI2CMaster(I2CSpeed speed = I2CSpeed::S400K);

    /**
     * @brief Записать данные на шину I2C
     * @param deviceAddress 7-битный адрес устройства-получателя
     * @param data Данные для передачи
     * @param flag Флаги транзакции (по умолчанию 0x02 - START)
     * @throw std::runtime_error Если устройство не открыто или не в режиме I2C
     * @throw std::runtime_error При ошибке записи или неполной передаче
     */
    void i2cMasterWrite(uint8_t deviceAddress, const std::vector<uint8_t> &data,
                        uint8_t flag = 0x02);

    /**
     * @brief Прочитать данные с шины I2C
     * @param deviceAddress 7-битный адрес устройства-отправителя
     * @param bytesToRead Количество байт для чтения
     * @param flag Флаги транзакции (по умолчанию 0x02 - START)
     * @return Вектор прочитанных байт
     * @throw std::runtime_error Если устройство не открыто или не в режиме I2C
     * @throw std::runtime_error При ошибке чтения
     */
    std::vector<uint8_t> i2cMasterRead(uint8_t deviceAddress, size_t bytesToRead,
                                       uint8_t flag = 0x02);

    /**
     * @brief Получить статус шины I2C
     * @return Байт состояния шины I2C
     * @throw std::runtime_error Если устройство не открыто или не в режиме I2C
     */
    uint8_t i2cMasterGetStatus();

    /**
     * @brief Выполнить сброс шины I2C
     * @throw std::runtime_error Если устройство не открыто или не в режиме I2C
     */
    void i2cMasterResetBus();

    /**
     * @brief Просканировать шину I2C и вернуть адреса устройств, ответивших ACK
     * @param startAddress Первый проверяемый адрес (по умолчанию 0x03)
     * @param endAddress Последний проверяемый адрес (по умолчанию 0x77)
     * @param flag Флаги транзакции (по умолчанию START|STOP = 0x03)
     * @return Вектор 7-битных адресов устройств, ответивших на запрос
     * @throw std::runtime_error Если устройство не открыто или не в режиме I2C
     *
     * @note Посылает нулевую передачу (только адрес) к каждому адресу и фиксирует,
     *       какие устройства возвращают подтверждение (ACK). Специальные/зарезервированные
     *       адреса по умолчанию пропускаются (сканируются 0x03–0x77).
     */
    std::vector<uint8_t> scanI2CBus(uint8_t startAddress = 0x03,
                                    uint8_t endAddress = 0x77,
                                    uint8_t flag = 0x06) const;

    // Функции SPI Master режима

    /**
     * @brief Инициализировать устройство в режиме SPI Master
     * @param mode Режим работы SPI (одиночный/двойной/четверной)
     * @param clockDiv Делитель частоты SPI
     * @param polarity Полярность тактового сигнала
     * @param phase Фаза тактового сигнала
     * @throw std::runtime_error Если устройство не открыто
     */
    void initSPIMaster(FT4222_SPIMode mode = SPI_IO_SINGLE,
                       SPIClockDivider clockDiv = SPIClockDivider::DIV_512,
                       FT4222_SPICPOL polarity = CLK_IDLE_LOW,
                       FT4222_SPICPHA phase = CLK_LEADING);

    /**
     * @brief Прочитать данные через SPI (без одновременной записи)
     * @param bytesToRead Количество байт для чтения
     * @param endTransaction Завершить транзакцию (освободить CS)
     * @return Вектор прочитанных байт
     * @throw std::runtime_error Если устройство не открыто или не в режиме SPI
     */
    std::vector<uint8_t> spiMasterSingleRead(size_t bytesToRead, bool endTransaction = true);

    /**
     * @brief Записать данные через SPI (без одновременного чтения)
     * @param data Данные для записи
     * @param endTransaction Завершить транзакцию (освободить CS)
     * @throw std::runtime_error Если устройство не открыто или не в режиме SPI
     * @throw std::runtime_error При неполной записи
     */
    void spiMasterSingleWrite(const std::vector<uint8_t> &data, bool endTransaction = true);

    /**
     * @brief Одновременная запись и чтение через SPI (full-duplex)
     * @param writeData Данные для записи
     * @param endTransaction Завершить транзакцию (освободить CS)
     * @return Вектор прочитанных байт (того же размера, что и writeData)
     * @throw std::runtime_error Если устройство не открыто или не в режиме SPI
     */
    std::vector<uint8_t> spiMasterSingleReadWrite(const std::vector<uint8_t> &writeData,
                                                  bool endTransaction = true);

    // Функции GPIO режима

    /**
     * @brief Инициализировать GPIO выводы
     * @param dir0 Направление вывода 0
     * @param dir1 Направление вывода 1
     * @param dir2 Направление вывода 2
     * @param dir3 Направление вывода 3
     * @throw std::runtime_error Если устройство не открыто
     */
    void initGPIO(GPIO_Dir dir0 = GPIO_INPUT, GPIO_Dir dir1 = GPIO_INPUT,
                  GPIO_Dir dir2 = GPIO_INPUT, GPIO_Dir dir3 = GPIO_INPUT);

    /**
     * @brief Прочитать состояние GPIO вывода
     * @param port Номер вывода (0-3)
     * @return true если высокий уровень, false если низкий
     * @throw std::runtime_error Если устройство не открыто
     */
    bool readGPIO(GPIO_Port port);

    /**
     * @brief Установить состояние GPIO вывода
     * @param port Номер вывода (0-3)
     * @param value true для высокого уровня, false для низкого
     * @throw std::runtime_error Если устройство не открыто
     */
    void writeGPIO(GPIO_Port port, bool value);

    // Управление настройками устройства

    /**
     * @brief Установить системную частоту FT4222
     * @param clkRate Значение частоты из FT4222_ClockRate
     * @throw std::runtime_error Если устройство не открыто
     */
    void setClockRate(FT4222_ClockRate clkRate);

    /**
     * @brief Получить текущую системную частоту FT4222
     * @return Текущая частота или SYS_CLK_60 если устройство не открыто
     */
    FT4222_ClockRate getClockRate() const;

    /**
     * @brief Выполнить программный сброс чипа
     * @throw std::runtime_error Если устройство не открыто
     */
    void resetChip();

    // Получение информации об устройстве

    /**
     * @brief Получить строку с версиями чипа и библиотеки
     * @return Строка вида "Chip: 0xXXXX, Lib: 0xYYYY" или пустая строка
     */
    std::string getVersionString() const;

    /**
     * @brief Получить текущий режим работы устройства
     * @return Текущий режим или Mode::Unknown
     */
    Mode getDeviceMode() const;

    /**
     * @brief Получить режим работы чипа (конфигурацию выводов)
     * @return Байт режима чипа или 0 при ошибке
     */
    uint8_t getChipMode() const;

private:
    // Структура для сокрытия деталей реализации (Pimpl идиома)
    struct Impl;
    Impl *pimpl = nullptr;
    Logger m_logger; ///< Функция для логирования (может быть nullptr)

    // Внутренние вспомогательные методы

    /**
     * @brief Записать сообщение в лог
     * @param s Сообщение для логирования
     */
    void log(const std::string &s) const;

    /**
     * @brief Проверить статус FT4222 и сгенерировать исключение при ошибке
     * @param status Код статуса FT4222_STATUS
     * @param operation Название операции для сообщения об ошибке
     * @throw std::runtime_error Если status != FT4222_OK
     */
    void checkFT4222Status(FT4222_STATUS status, const std::string &operation) const;

    /**
     * @brief Проверить статус FT и сгенерировать исключение при ошибке
     * @param status Код статуса FT_STATUS
     * @param operation Название операции для сообщения об ошибке
     * @throw FtException Если status != FT_OK
     */
    void checkFTStatus(FT_STATUS status, const std::string &operation) const;
};
