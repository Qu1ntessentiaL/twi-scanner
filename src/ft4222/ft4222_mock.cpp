// Stub FT4222 backend for CI / builds without LibFT4222 (no real hardware access).

#include "ft4222.hpp"

#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>

struct FTDevice::Impl {
    bool open = false;
    std::string serial;
    uint32_t index = std::numeric_limits<uint32_t>::max();
    FT4222_ClockRate clockRate = SYS_CLK_60;
    mutable FTDevice::Mode currentMode = FTDevice::Mode::Unknown;
    std::mutex deviceMutex;
    bool gpioOut[4] = {};
};

std::vector<DeviceInfo> DeviceEnumerator::listDevices() {
    DeviceInfo mock;
    mock.index = 0;
    mock.serial = "MOCK0001";
    mock.description = "FT4222 mock device (no LibFT4222)";
    mock.locationId = 0;
    mock.flags = 0;
    return {mock};
}

void DeviceEnumerator::printDevices() {
    std::cout << "Mock mode: LibFT4222 is not linked. Showing a placeholder device.\n";
    std::cout << "Install FTDI LibFT4222 for real hardware (see README).\n\n";
    for (const auto &dev : listDevices()) {
        std::cout << "Index      : " << dev.index << "\n"
                  << "Serial     : " << dev.serial << "\n"
                  << "Description: " << dev.description << "\n"
                  << "LocationId : " << dev.locationId << "\n\n";
    }
}

FTDevice::FTDevice(FTDevice &&other) noexcept
    : pimpl(std::move(other.pimpl)), m_logger(std::move(other.m_logger)) {}

FTDevice &FTDevice::operator=(FTDevice &&other) noexcept {
    if (this != &other) {
        close();
        pimpl = std::move(other.pimpl);
        m_logger = std::move(other.m_logger);
    }
    return *this;
}

FTDevice::FTDevice(uint32_t index, Logger logger)
    : pimpl(std::make_unique<Impl>()), m_logger(std::move(logger)) {
    if (index != std::numeric_limits<uint32_t>::max())
        open(index);
}

FTDevice::FTDevice(const std::string &serialNumber, Logger logger)
    : pimpl(std::make_unique<Impl>()), m_logger(std::move(logger)) {
    openBySerial(serialNumber);
}

FTDevice::~FTDevice() {
    try {
        close();
    } catch (...) {
        log("Exception in destructor while closing device");
    }
}

void FTDevice::open(uint32_t index) {
    if (!pimpl)
        pimpl = std::make_unique<Impl>();
    if (isOpen())
        throw std::runtime_error("Device is already open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);
    pimpl->open = true;
    pimpl->index = index;
    pimpl->serial.clear();
    pimpl->currentMode = Mode::Unknown;
    log("Mock device opened index=" + std::to_string(index));
}

void FTDevice::openBySerial(const std::string &serialNumber) {
    if (!pimpl)
        pimpl = std::make_unique<Impl>();
    if (isOpen())
        throw std::runtime_error("Device is already open");

    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);
    pimpl->open = true;
    pimpl->serial = serialNumber;
    pimpl->currentMode = Mode::Unknown;
    log("Mock device opened serial=" + serialNumber);
}

void FTDevice::close() noexcept {
    if (!pimpl || !pimpl->open)
        return;
    std::lock_guard<std::mutex> lock(pimpl->deviceMutex);
    pimpl->open = false;
    pimpl->currentMode = Mode::Unknown;
    log("Mock device closed");
}

bool FTDevice::isOpen() const noexcept {
    return pimpl && pimpl->open;
}

std::vector<uint8_t> FTDevice::read(size_t bytesToRead, unsigned int) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    return std::vector<uint8_t>(bytesToRead, 0);
}

void FTDevice::write(const std::vector<uint8_t> &data, unsigned int) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (data.empty())
        return;
    log("Mock write " + std::to_string(data.size()) + " bytes");
}

void FTDevice::initI2CMaster(I2CSpeed speed) const {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    pimpl->currentMode = Mode::I2C_Master;
    log("Mock I2C init " + std::to_string(static_cast<int>(speed)) + " kbps");
}

void FTDevice::i2cMasterWrite(uint8_t deviceAddress, const std::vector<uint8_t> &data,
                              uint8_t) const {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master)
        throw std::runtime_error("Device not in I2C Master mode");
    if (data.empty())
        return;
    std::ostringstream oss;
    oss << "Mock I2C write addr=0x" << std::hex << static_cast<int>(deviceAddress) << std::dec
        << " len=" << data.size();
    log(oss.str());
}

std::vector<uint8_t> FTDevice::i2cMasterRead(uint8_t deviceAddress, size_t bytesToRead, uint8_t) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master)
        throw std::runtime_error("Device not in I2C Master mode");
    log("Mock I2C read addr=0x" + std::to_string(deviceAddress));
    return std::vector<uint8_t>(bytesToRead, 0);
}

uint8_t FTDevice::i2cMasterGetStatus() {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    return 0;
}

void FTDevice::i2cMasterResetBus() {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    log("Mock I2C bus reset");
}

std::vector<uint8_t> FTDevice::scanI2CBus(uint8_t startAddress, uint8_t endAddress,
                                          uint8_t) const {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::I2C_Master)
        throw std::runtime_error("Device not in I2C Master mode");
    if (startAddress > endAddress)
        std::swap(startAddress, endAddress);
    (void)startAddress;
    (void)endAddress;
    log("Mock I2C scan (no devices)");
    return {};
}

void FTDevice::initSPIMaster(FT4222_SPIMode, SPIClockDivider, FT4222_SPICPOL, FT4222_SPICPHA) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    pimpl->currentMode = Mode::SPI_Master;
    log("Mock SPI initialized");
}

std::vector<uint8_t> FTDevice::spiMasterSingleRead(size_t bytesToRead, bool) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::SPI_Master)
        throw std::runtime_error("Device not in SPI Master mode");
    return std::vector<uint8_t>(bytesToRead, 0);
}

void FTDevice::spiMasterSingleWrite(const std::vector<uint8_t> &data, bool) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::SPI_Master)
        throw std::runtime_error("Device not in SPI Master mode");
    log("Mock SPI write " + std::to_string(data.size()) + " bytes");
}

std::vector<uint8_t> FTDevice::spiMasterSingleReadWrite(const std::vector<uint8_t> &writeData,
                                                        bool) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    if (pimpl->currentMode != Mode::SPI_Master)
        throw std::runtime_error("Device not in SPI Master mode");
    return std::vector<uint8_t>(writeData.size(), 0);
}

void FTDevice::initGPIO(GPIO_Dir, GPIO_Dir, GPIO_Dir, GPIO_Dir) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    pimpl->currentMode = Mode::GPIO;
    log("Mock GPIO initialized");
}

bool FTDevice::readGPIO(GPIO_Port port) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    const auto p = static_cast<int>(port);
    if (p < 0 || p > 3)
        throw std::runtime_error("Invalid GPIO port");
    return pimpl->gpioOut[p];
}

void FTDevice::writeGPIO(GPIO_Port port, bool value) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    const auto p = static_cast<int>(port);
    if (p < 0 || p > 3)
        throw std::runtime_error("Invalid GPIO port");
    pimpl->gpioOut[p] = value;
}

void FTDevice::setClockRate(FT4222_ClockRate clkRate) {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    pimpl->clockRate = clkRate;
}

FT4222_ClockRate FTDevice::getClockRate() const {
    if (!isOpen())
        return SYS_CLK_60;
    return pimpl->clockRate;
}

void FTDevice::resetChip() {
    if (!isOpen())
        throw std::runtime_error("Device not open");
    pimpl->currentMode = Mode::Unknown;
    log("Mock chip reset");
}

std::string FTDevice::getVersionString() const {
    if (!isOpen())
        return "";
    return "Chip: mock, Lib: mock";
}

FTDevice::Mode FTDevice::getDeviceMode() const {
    return pimpl ? pimpl->currentMode : Mode::Unknown;
}

uint8_t FTDevice::getChipMode() const {
    return isOpen() ? 0 : 0;
}

void FTDevice::log(const std::string &s) const {
    if (m_logger)
        m_logger(s);
}
