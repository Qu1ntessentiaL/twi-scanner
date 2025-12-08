#include "ft4222.hpp"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>

/**
 * @brief Внутренняя структура для сокрытия данных устройства (Pimpl идиома)
 *
 * Содержит все низкоуровневые хэндлы и состояние устройства.
 * Изоляция позволяет менять реализацию без изменения публичного интерфейса.
 */
struct FTDevice::Impl {
    FT_HANDLE ftHandle = nullptr; ///< Хэндл открытого FTDI-устройства
    FT4222_ClockRate clockRate = SYS_CLK_60; ///< Текущая системная частота
    Mode currentMode = Mode::Unknown; ///< Текущий режим работы
    bool isFt4222 = false; ///< Флаг, что это именно FT4222
    uint32_t openedIndex = std::numeric_limits<uint32_t>::max(); ///< Индекс открытого устройства
    std::mutex deviceMutex; ///< Мьютекс для потокобезопасности
};

// DeviceEnumerator

/**
 * @brief Получить список всех подключенных FT4222-устройств
 * @return Вектор с информацией о каждом устройстве
 * @throw FtException При ошибке получения списка устройств
 *
 * Метод фильтрует только устройства типа FT4222H (три варианта конфигурации).
 * Для каждого устройства сохраняется серийный номер, описание, индексы и флаги.
 */
std::vector<DeviceInfo> DeviceEnumerator::listDevices() {
    std::vector<DeviceInfo> out;

    DWORD numDevices = 0;
    // Создаем список устройств и получаем их количество
    FT_STATUS status = FT_CreateDeviceInfoList(&numDevices);

    if (status != FT_OK) {
        throw FtException("Failed to create device info list", status);
    }

    if (numDevices == 0) {
        return out; // Возвращаем пустой вектор
    }

    // Выделяем память для массива структур с информацией об устройствах
    FT_DEVICE_LIST_INFO_NODE *devices = new FT_DEVICE_LIST_INFO_NODE[numDevices];
    status = FT_GetDeviceInfoList(devices, &numDevices);

    if (status == FT_OK) {
        for (DWORD i = 0; i < numDevices; i++) {
            // Проверяем, является ли устройство FT4222 по его типу
            bool isFt4222 = false;
            if (devices[i].Type == FT_DEVICE_4222H_0 || // Режим 0
                devices[i].Type == FT_DEVICE_4222H_1_2 || // Режим 1 или 2
                devices[i].Type == FT_DEVICE_4222H_3) {
                // Режим 3
                isFt4222 = true;
            }

            if (isFt4222) {
                DeviceInfo di;
                di.index = static_cast<uint32_t>(i);
                di.serial = devices[i].SerialNumber;
                di.description = devices[i].Description;
                di.locationId = devices[i].LocId;
                di.flags = devices[i].Flags;
                out.push_back(di);
            }
        }
    }

    delete[] devices; // Освобождаем выделенную память

    if (status != FT_OK) {
        throw FtException("Failed to get device info list", status);
    }

    return out;
}

// ---------- Вспомогательные методы проверки статуса ----------

/**
 * @brief Проверить статус FT4222 и сгенерировать исключение при ошибке
 * @param status Код статуса FT4222_STATUS
 * @param operation Название операции для сообщения об ошибке
 * @throw std::runtime_error Если status != FT4222_OK
 */
void FTDevice::checkFT4222Status(FT4222_STATUS status, const std::string &operation) const {
    if (status != FT4222_OK) {
        std::ostringstream oss;
        oss << operation << " failed with FT4222_STATUS: " << status;
        throw std::runtime_error(oss.str());
    }
}

/**
 * @brief Проверить статус FT и сгенерировать исключение при ошибке
 * @param status Код статуса FT_STATUS
 * @param operation Название операции для сообщения об ошибке
 * @throw FtException Если status != FT_OK
 */
void FTDevice::checkFTStatus(FT_STATUS status, const std::string &operation) const {
    if (status != FT_OK) {
        throw FtException(operation + " failed", status);
    }
}

// Конструкторы и деструктор

/**
 * @brief Конструктор перемещения
 * @param other Объект для перемещения
 *
 * Перемещает владение ресурсами (хэндлы устройства) от другого объекта.
 * Исходный объект становится невалидным (pimpl = nullptr).
 */
FTDevice::FTDevice(FTDevice &&other) noexcept
    : pimpl(other.pimpl), m_logger(std::move(other.m_logger)) {
    other.pimpl = nullptr; // Запрещаем двойное освобождение
}

/**
 * @brief Оператор присваивания перемещением
 * @param other Объект для перемещения
 * @return Ссылка на текущий объект
 *
 * Закрывает текущее устройство (если открыто) и перемещает ресурсы.
 */
FTDevice &FTDevice::operator=(FTDevice &&other) noexcept {
    if (this != &other) {
        close(); // Закрываем текущее устройство
        delete pimpl;
        pimpl = other.pimpl;
        m_logger = std::move(other.m_logger);
        other.pimpl = nullptr; // Запрещаем двойное освобождение
    }
    return *this;
}

/**
 * @brief Конструктор с открытием по индексу
 * @param index Индекс устройства в списке FTDI
 * @param logger Функция для логирования
 *
 * Если index != UINT32_MAX, сразу открывает устройство.
 * Иначе создает объект для последующего открытия.
 */
FTDevice::FTDevice(uint32_t index, Logger logger)
    : pimpl(new Impl()), m_logger(std::move(logger)) {
    if (index != std::numeric_limits<uint32_t>::max()) {
        open(index); // Открываем устройство по индексу
    }
}

/**
 * @brief Конструктор с открытием по серийному номеру
 * @param serialNumber Серийный номер устройства
 * @param logger Функция для логирования
 */
FTDevice::FTDevice(const std::string &serialNumber, Logger logger)
    : pimpl(new Impl()), m_logger(std::move(logger)) {
    openBySerial(serialNumber); // Открываем устройство по серийному номеру
}

/**
 * @brief Деструктор
 *
 * Закрывает устройство и освобождает ресурсы.
 * Обрабатывает исключения в деструкторе, чтобы избежать аварийного завершения.
 */
FTDevice::~FTDevice() {
    try {
        close(); // Пытаемся корректно закрыть устройство
    }
    catch (...) {
        log("Exception in destructor while closing device");
    }
    delete pimpl;
    pimpl = nullptr;
}

// Основные операции с устройством

/**
 * @brief Открыть устройство по индексу в списке FTDI
 * @param index Индекс устройства
 * @throw std::runtime_error Если устройство уже открыто
 * @throw FtException При ошибке открытия
 * @throw std::runtime_error Если устройство не FT4222
 *
 * Выполняет полное открытие устройства: получение хэндла,
 * проверка типа устройства, получение информации о версии.
 */
void FTDevice::open(uint32_t index) {
    if (!pimpl) pimpl = new Impl(); // Создаем Impl, если еще не создан

    if (isOpen()) {
        throw std::runtime_error("Device is already open");
    }

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex); // Потокобезопасность

    // Открываем устройство по индексу
    FT_STATUS status = FT_Open(static_cast<int>(index), &pimpl->ftHandle);
    checkFTStatus(status, "FT_Open");

    // Проверяем, что это действительно FT4222
    FT_DEVICE deviceType;
    DWORD deviceId;
    char serialNumber[16];
    char description[64];

    status = FT_GetDeviceInfo(pimpl->ftHandle, &deviceType, &deviceId,
                              serialNumber, description, nullptr);
    checkFTStatus(status, "FT_GetDeviceInfo");

    // Проверяем тип устройства (три возможных варианта FT4222H)
    if (deviceType != FT_DEVICE_4222H_0 &&
        deviceType != FT_DEVICE_4222H_1_2 &&
        deviceType != FT_DEVICE_4222H_3) {
        FT_Close(pimpl->ftHandle); // Закрываем, если не FT4222
        throw std::runtime_error("Device is not FT4222");
    }

    pimpl->isFt4222 = true;
    pimpl->openedIndex = index;

    // Получаем версию библиотеки и чипа для отладки
    FT4222_Version version;
    FT4222_STATUS ft4222Status = FT4222_GetVersion(pimpl->ftHandle, &version);
    if (ft4222Status == FT4222_OK) {
        std::ostringstream oss;
        oss << "FT4222 Chip: 0x" << std::hex << version.chipVersion
            << ", Lib: 0x" << version.dllVersion << std::dec;
        log(oss.str());
    }

    log("Device opened index=" + std::to_string(index));
}

/**
 * @brief Открыть устройство по серийному номеру
 * @param serialNumber Серийный номер устройства
 * @throw std::runtime_error Если устройство уже открыто
 * @throw FtException При ошибке открытия
 *
 * Альтернативный способ открытия устройства, полезный когда
 * известно точное серийное номер конкретного устройства.
 */
void FTDevice::openBySerial(const std::string &serialNumber) {
    if (!pimpl) pimpl = new Impl();

    if (isOpen()) {
        throw std::runtime_error("Device is already open");
    }

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    // Открываем устройство по серийному номеру
    FT_STATUS status = FT_OpenEx((PVOID)serialNumber.c_str(),
                                 FT_OPEN_BY_SERIAL_NUMBER,
                                 &pimpl->ftHandle);
    checkFTStatus(status, "FT_OpenEx by serial");

    pimpl->isFt4222 = true;
    log("Device opened by serial: " + serialNumber);
}

/**
 * @brief Закрыть устройство и освободить ресурсы
 *
 * Выполняет деинициализацию FT4222 и закрытие хэндла FTDI.
 * Гарантированно не бросает исключений (noexcept).
 */
void FTDevice::close() noexcept {
    if (!pimpl || !pimpl->ftHandle) return; // Уже закрыто

    try {
        std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

        if (pimpl->isFt4222) {
            // Деинициализируем FT4222 перед закрытием
            FT4222_UnInitialize(pimpl->ftHandle);
        }

        // Закрываем хэндл FTDI
        FT_STATUS status = FT_Close(pimpl->ftHandle);
        if (status != FT_OK && m_logger) {
            log("FT_Close failed with status: " + std::to_string(status));
        }

        // Сбрасываем состояние
        pimpl->ftHandle = nullptr;
        pimpl->currentMode = Mode::Unknown;
        pimpl->isFt4222 = false;
        log("Device closed");
    }
    catch (...) {
        if (m_logger) {
            log("Exception during close");
        }
    }
}

/**
 * @brief Проверить, открыто ли устройство
 * @return true если устройство открыто и это FT4222
 */
bool FTDevice::isOpen() const noexcept {
    return pimpl && pimpl->ftHandle != nullptr && pimpl->isFt4222;
}

// I2C Master функции

/**
 * @brief Инициализировать устройство в режиме I2C Master
 * @param speed Скорость шины I2C
 * @throw std::runtime_error Если устройство не открыто или ошибка инициализации
 *
 * Устанавливает устройство в режим I2C ведущего с указанной скоростью.
 * После инициализации можно выполнять операции чтения/записи на шине I2C.
 */
void FTDevice::initI2CMaster(I2CSpeed speed) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    // Инициализируем I2C Master с указанной скоростью
    FT4222_STATUS status = FT4222_I2CMaster_Init(pimpl->ftHandle,
                                                 static_cast<uint32>(speed));
    checkFT4222Status(status, "FT4222_I2CMaster_Init");

    pimpl->currentMode = Mode::I2C_Master; // Устанавливаем текущий режим

    std::ostringstream oss;
    oss << "I2C Master initialized at " << static_cast<int>(speed) << " kbps";
    log(oss.str());
}

/**
 * @brief Записать данные на шину I2C
 * @param deviceAddress 7-битный адрес устройства
 * @param data Данные для передачи
 * @param flag Флаги транзакции
 * @throw std::runtime_error При ошибках устройства или неполной передаче
 *
 * Выполняет запись данных на шину I2C с указанным адресом устройства.
 * Флаг определяет условия начала/окончания транзакции.
 */
void FTDevice::i2cMasterWrite(uint8_t deviceAddress, const std::vector<uint8_t> &data,
                              uint8_t flag) {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master) {
        throw std::runtime_error("Device not in I2C Master mode");
    }

    if (data.empty()) return; // Нет данных для записи

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    uint16 bytesWritten = 0;
    // Выполняем запись на шину I2C
    FT4222_STATUS status = FT4222_I2CMaster_WriteEx(pimpl->ftHandle,
                                                    deviceAddress,
                                                    flag,
                                                    const_cast<uint8*>(data.data()),
                                                    static_cast<uint16>(data.size()),
                                                    &bytesWritten);

    checkFT4222Status(status, "FT4222_I2CMaster_WriteEx");

    // Проверяем, что переданы все данные
    if (bytesWritten != data.size()) {
        std::ostringstream oss;
        oss << "I2C Master Write incomplete. Written: " << bytesWritten
            << "/" << data.size() << " bytes";
        throw std::runtime_error(oss.str());
    }

    // Логируем успешную операцию
    std::ostringstream oss;
    oss << "I2C Write to 0x" << std::hex << static_cast<int>(deviceAddress)
        << std::dec << ": " << bytesWritten << " bytes, flag=0x"
        << std::hex << static_cast<int>(flag) << std::dec;
    log(oss.str());
}

/**
 * @brief Прочитать данные с шины I2C
 * @param deviceAddress 7-битный адрес устройства
 * @param bytesToRead Количество байт для чтения
 * @param flag Флаги транзакции
 * @return Вектор прочитанных данных
 * @throw std::runtime_error При ошибках устройства
 *
 * Выполняет чтение данных с шины I2C с указанного устройства.
 * Возвращает фактически прочитанные данные (может быть меньше запрошенного).
 */
std::vector<uint8_t> FTDevice::i2cMasterRead(uint8_t deviceAddress, size_t bytesToRead,
                                             uint8_t flag) {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master) {
        throw std::runtime_error("Device not in I2C Master mode");
    }

    if (bytesToRead == 0) return {}; // Нет данных для чтения

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    std::vector<uint8_t> buffer(bytesToRead);
    uint16 bytesRead = 0;

    // Выполняем чтение с шины I2C
    FT4222_STATUS status = FT4222_I2CMaster_ReadEx(pimpl->ftHandle,
                                                   deviceAddress,
                                                   flag,
                                                   buffer.data(),
                                                   static_cast<uint16>(bytesToRead),
                                                   &bytesRead);

    checkFT4222Status(status, "FT4222_I2CMaster_ReadEx");

    // Если прочитано не все, уменьшаем размер буфера
    if (bytesRead != bytesToRead) {
        buffer.resize(bytesRead);
        log("I2C Read incomplete: " + std::to_string(bytesRead) + "/" +
            std::to_string(bytesToRead) + " bytes");
    }

    // Логируем успешную операцию
    std::ostringstream oss;
    oss << "I2C Read from 0x" << std::hex << static_cast<int>(deviceAddress)
        << std::dec << ": " << bytesRead << " bytes, flag=0x"
        << std::hex << static_cast<int>(flag) << std::dec;
    log(oss.str());

    return buffer;
}

/**
 * @brief Получить статус шины I2C
 * @return Байт состояния шины
 * @throw std::runtime_error При ошибке получения статуса
 *
 * Возвращает текущее состояние шины I2C (занята/свободна, ошибки и т.д.).
 */
uint8_t FTDevice::i2cMasterGetStatus() {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master) {
        throw std::runtime_error("Device not in I2C Master mode");
    }

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    uint8 status = 0;
    FT4222_STATUS ftStatus = FT4222_I2CMaster_GetStatus(pimpl->ftHandle, &status);
    checkFT4222Status(ftStatus, "FT4222_I2CMaster_GetStatus");

    return status;
}

/**
 * @brief Сбросить шину I2C
 * @throw std::runtime_error При ошибке сброса
 *
 * Выполняет сброс состояния шины I2C. Полезно при зависании шины
 * или необходимости восстановления после ошибок.
 */
void FTDevice::i2cMasterResetBus() {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master) {
        throw std::runtime_error("Device not in I2C Master mode");
    }

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    FT4222_STATUS status = FT4222_I2CMaster_ResetBus(pimpl->ftHandle);
    checkFT4222Status(status, "FT4222_I2CMaster_ResetBus");

    log("I2C bus reset");
}

// SPI Master функции

/**
 * @brief Инициализировать устройство в режиме SPI Master
 * @param mode Режим работы SPI
 * @param clockDiv Делитель частоты
 * @param polarity Полярность тактового сигнала
 * @param phase Фаза тактового сигнала
 * @throw std::runtime_error При ошибках устройства
 *
 * Настраивает устройство как SPI ведущий с указанными параметрами.
 * По умолчанию используется одиночный режим, делитель 512, стандартные полярность и фаза.
 */
void FTDevice::initSPIMaster(FT4222_SPIMode mode, SPIClockDivider clockDiv,
                             FT4222_SPICPOL polarity, FT4222_SPICPHA phase) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    // Инициализируем SPI Master
    FT4222_STATUS status = FT4222_SPIMaster_Init(pimpl->ftHandle,
                                                 mode,
                                                 static_cast<FT4222_SPIClock>(clockDiv),
                                                 polarity,
                                                 phase,
                                                 0x01); // ssoMap: только CS0 активен
    checkFT4222Status(status, "FT4222_SPIMaster_Init");

    pimpl->currentMode = Mode::SPI_Master;
    log("SPI Master initialized");
}

/**
 * @brief Прочитать данные через SPI (без записи)
 * @param bytesToRead Количество байт для чтения
 * @param endTransaction Завершить транзакцию после чтения
 * @return Вектор прочитанных данных
 * @throw std::runtime_error При ошибках устройства
 *
 * Выполняет только чтение по SPI. Полезно для устройств, которые
 * самостоятельно инициируют передачу данных.
 */
std::vector<uint8_t> FTDevice::spiMasterSingleRead(size_t bytesToRead, bool endTransaction) {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::SPI_Master) {
        throw std::runtime_error("Device not in SPI Master mode");
    }

    if (bytesToRead == 0) return {};

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    std::vector<uint8_t> buffer(bytesToRead);
    uint16 bytesRead = 0;

    // Выполняем чтение по SPI
    FT4222_STATUS status = FT4222_SPIMaster_SingleRead(pimpl->ftHandle,
                                                       buffer.data(),
                                                       static_cast<uint16>(bytesToRead),
                                                       &bytesRead,
                                                       endTransaction ? TRUE : FALSE);
    checkFT4222Status(status, "FT4222_SPIMaster_SingleRead");

    if (bytesRead != bytesToRead) {
        buffer.resize(bytesRead); // Корректируем размер при неполном чтении
    }

    log("SPI SingleRead: " + std::to_string(bytesRead) + " bytes");
    return buffer;
}

/**
 * @brief Записать данные через SPI (без чтения)
 * @param data Данные для записи
 * @param endTransaction Завершить транзакцию после записи
 * @throw std::runtime_error При ошибках устройства или неполной записи
 *
 * Выполняет только запись по SPI. Полезно для конфигурации устройств
 * или передачи команд без ожидания ответа.
 */
void FTDevice::spiMasterSingleWrite(const std::vector<uint8_t> &data, bool endTransaction) {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::SPI_Master) {
        throw std::runtime_error("Device not in SPI Master mode");
    }

    if (data.empty()) return;

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    uint16 bytesWritten = 0;
    // Выполняем запись по SPI
    FT4222_STATUS status = FT4222_SPIMaster_SingleWrite(pimpl->ftHandle,
                                                        const_cast<uint8*>(data.data()),
                                                        static_cast<uint16>(data.size()),
                                                        &bytesWritten,
                                                        endTransaction ? TRUE : FALSE);
    checkFT4222Status(status, "FT4222_SPIMaster_SingleWrite");

    // Проверяем полную запись
    if (bytesWritten != data.size()) {
        throw std::runtime_error("SPI Write incomplete: " +
            std::to_string(bytesWritten) + "/" +
            std::to_string(data.size()) + " bytes");
    }

    log("SPI SingleWrite: " + std::to_string(bytesWritten) + " bytes");
}

/**
 * @brief Одновременная запись и чтение по SPI
 * @param writeData Данные для записи
 * @param endTransaction Завершить транзакцию после операции
 * @return Вектор прочитанных данных
 * @throw std::runtime_error При ошибках устройства
 *
 * Выполняет полнодуплексную операцию SPI: одновременно запись и чтение.
 * Размер возвращаемых данных равен размеру передаваемых данных.
 */
std::vector<uint8_t> FTDevice::spiMasterSingleReadWrite(const std::vector<uint8_t> &writeData,
                                                        bool endTransaction) {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::SPI_Master) {
        throw std::runtime_error("Device not in SPI Master mode");
    }

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    std::vector<uint8_t> readBuffer(writeData.size());
    uint16 bytesTransferred = 0;

    // Выполняем одновременную запись и чтение
    FT4222_STATUS status = FT4222_SPIMaster_SingleReadWrite(pimpl->ftHandle,
                                                            readBuffer.data(),
                                                            const_cast<uint8*>(writeData.data()),
                                                            static_cast<uint16>(writeData.size()),
                                                            &bytesTransferred,
                                                            endTransaction ? TRUE : FALSE);
    checkFT4222Status(status, "FT4222_SPIMaster_SingleReadWrite");

    if (bytesTransferred != writeData.size()) {
        readBuffer.resize(bytesTransferred); // Корректируем размер
    }

    log("SPI SingleReadWrite: " + std::to_string(bytesTransferred) + " bytes");
    return readBuffer;
}

// GPIO функции

/**
 * @brief Инициализировать GPIO выводы
 * @param dir0 Направление вывода 0 (GPIO0)
 * @param dir1 Направление вывода 1 (GPIO1)
 * @param dir2 Направление вывода 2 (GPIO2)
 * @param dir3 Направление вывода 3 (GPIO3)
 * @throw std::runtime_error При ошибках устройства
 *
 * Настраивает направление (вход/выход) для каждого из четырех GPIO выводов.
 * По умолчанию все выводы настроены как входы.
 */
void FTDevice::initGPIO(GPIO_Dir dir0, GPIO_Dir dir1, GPIO_Dir dir2, GPIO_Dir dir3) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    GPIO_Dir dirs[4] = {dir0, dir1, dir2, dir3};
    FT4222_STATUS status = FT4222_GPIO_Init(pimpl->ftHandle, dirs);
    checkFT4222Status(status, "FT4222_GPIO_Init");

    pimpl->currentMode = Mode::GPIO;
    log("GPIO initialized");
}

/**
 * @brief Прочитать состояние GPIO вывода
 * @param port Номер вывода (0-3)
 * @return true если высокий уровень, false если низкий
 * @throw std::runtime_error При ошибках устройства
 */
bool FTDevice::readGPIO(GPIO_Port port) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    BOOL value = FALSE;
    FT4222_STATUS status = FT4222_GPIO_Read(pimpl->ftHandle, port, &value);
    checkFT4222Status(status, "FT4222_GPIO_Read");

    return value != FALSE;
}

/**
 * @brief Установить состояние GPIO вывода
 * @param port Номер вывода (0-3)
 * @param value true для высокого уровня, false для низкого
 * @throw std::runtime_error При ошибках устройства
 *
 * Устанавливает логический уровень на указанном GPIO выводе.
 * Вывод должен быть предварительно настроен как выход.
 */
void FTDevice::writeGPIO(GPIO_Port port, bool value) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    FT4222_STATUS status = FT4222_GPIO_Write(pimpl->ftHandle, port,
                                             value ? TRUE : FALSE);
    checkFT4222Status(status, "FT4222_GPIO_Write");

    std::ostringstream oss;
    oss << "GPIO Port" << static_cast<int>(port) << " set to "
        << (value ? "HIGH" : "LOW");
    log(oss.str());
}

// Общие функции

/**
 * @brief Прочитать данные через интерфейс D2XX
 * @param bytesToRead Количество байт для чтения
 * @param timeoutMs Таймаут операции в миллисекундах
 * @return Вектор прочитанных данных
 * @throw std::runtime_error Если устройство не открыто
 * @throw FtException При ошибке чтения
 *
 * Использует низкоуровневый FT_Read для чтения сырых данных.
 * Подходит для обмена данными в нестандартных режимах.
 */
std::vector<uint8_t> FTDevice::read(size_t bytesToRead, unsigned int timeoutMs) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    if (bytesToRead == 0) return {};

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    std::vector<uint8_t> buffer(bytesToRead);
    DWORD bytesRead = 0;

    // Устанавливаем таймаут чтения
    FT_SetTimeouts(pimpl->ftHandle, timeoutMs, timeoutMs);

    FT_STATUS status = FT_Read(pimpl->ftHandle, buffer.data(),
                               static_cast<DWORD>(bytesToRead), &bytesRead);
    checkFTStatus(status, "FT_Read");

    if (bytesRead != bytesToRead) {
        buffer.resize(bytesRead); // Корректируем размер при неполном чтении
        log("Read partial: " + std::to_string(bytesRead) + "/" +
            std::to_string(bytesToRead) + " bytes");
    }
    else {
        log("Read " + std::to_string(bytesRead) + " bytes");
    }

    return buffer;
}

/**
 * @brief Записать данные через интерфейс D2XX
 * @param data Данные для записи
 * @param timeoutMs Таймаут операции в миллисекундах
 * @throw std::runtime_error Если устройство не открыто
 * @throw FtException При ошибке записи или неполной передаче
 *
 * Использует низкоуровневый FT_Write для записи сырых данных.
 * Создает копию данных для обхода ограничений константности FT_Write.
 */
void FTDevice::write(const std::vector<uint8_t> &data, unsigned int timeoutMs) {
    if (!isOpen()) throw std::runtime_error("Device not open");
    if (data.empty()) return;

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    DWORD bytesWritten = 0;

    // Устанавливаем таймаут записи
    FT_SetTimeouts(pimpl->ftHandle, timeoutMs, timeoutMs);

    // Создаем неконстантную копию данных (требование FT_Write)
    std::vector<uint8_t> writeBuffer = data;

    FT_STATUS status = FT_Write(pimpl->ftHandle,
                                writeBuffer.data(),
                                static_cast<DWORD>(data.size()),
                                &bytesWritten);

    // Проверяем успешность записи
    if (status != FT_OK || bytesWritten != data.size()) {
        std::ostringstream oss;
        oss << "Write failed. Written: " << bytesWritten
            << "/" << data.size() << " bytes";
        throw FtException(oss.str(), status);
    }

    log("Write " + std::to_string(bytesWritten) + " bytes");
}

/**
 * @brief Установить системную частоту FT4222
 * @param clkRate Значение частоты
 * @throw std::runtime_error Если устройство не открыто
 *
 * Изменяет системную частоту FT4222, что влияет на скорость
 * всех интерфейсов (I2C, SPI) и максимальную производительность.
 */
void FTDevice::setClockRate(FT4222_ClockRate clkRate) {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    FT4222_STATUS status = FT4222_SetClock(pimpl->ftHandle, clkRate);
    checkFT4222Status(status, "FT4222_SetClock");

    pimpl->clockRate = clkRate; // Сохраняем текущую частоту

    log("Clock rate set to " + std::to_string(static_cast<int>(clkRate)));
}

/**
 * @brief Получить текущую системную частоту
 * @return Текущая частота или SYS_CLK_60 по умолчанию
 */
FT4222_ClockRate FTDevice::getClockRate() const {
    if (!isOpen()) return SYS_CLK_60;

    return pimpl->clockRate;
}

/**
 * @brief Выполнить программный сброс чипа
 * @throw std::runtime_error Если устройство не открыто
 *
 * Инициирует полный сброс FT4222. Устройство возвращается
 * в исходное состояние, все настройки сбрасываются.
 */
void FTDevice::resetChip() {
    if (!isOpen()) throw std::runtime_error("Device not open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    FT4222_STATUS status = FT4222_ChipReset(pimpl->ftHandle);
    checkFT4222Status(status, "FT4222_ChipReset");

    log("Chip reset");
}

/**
 * @brief Получить строку с версиями чипа и библиотеки
 * @return Строка формата "Chip: 0xXXXX, Lib: 0xYYYY" или пустая строка
 */
std::string FTDevice::getVersionString() const {
    if (!isOpen() || !pimpl->isFt4222) return "";

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    FT4222_Version version;
    FT4222_STATUS status = FT4222_GetVersion(pimpl->ftHandle, &version);

    if (status != FT4222_OK) return "";

    std::ostringstream oss;
    oss << "Chip: 0x" << std::hex << version.chipVersion
        << ", Lib: 0x" << version.dllVersion << std::dec;
    return oss.str();
}

/**
 * @brief Получить текущий режим работы устройства
 * @return Текущий режим или Mode::Unknown
 */
FTDevice::Mode FTDevice::getDeviceMode() const {
    return pimpl ? pimpl->currentMode : Mode::Unknown;
}

/**
 * @brief Получить режим работы чипа
 * @return Байт режима чипа или 0 при ошибке
 *
 * Возвращает конфигурацию выводов FT4222, определяющую
 * какие интерфейсы доступны в текущем режиме.
 */
uint8_t FTDevice::getChipMode() const {
    if (!isOpen() || !pimpl->isFt4222) return 0;

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);

    uint8 chipMode = 0;
    FT4222_STATUS status = FT4222_GetChipMode(pimpl->ftHandle, &chipMode);

    if (status != FT4222_OK) return 0;

    return chipMode;
}

/**
 * @brief Записать сообщение в лог
 * @param s Сообщение для логирования
 *
 * Вызывает функцию-логгер, если она установлена.
 * Игнорирует вызовы, если логгер не установлен (nullptr).
 */
void FTDevice::log(const std::string &s) const {
    if (m_logger) m_logger(s);
}
