#pragma once

#include "ft4222/ft4222.hpp"

enum class DeviceMode {
    None,
    I2C,
    SPI,
    GPIO
};

struct AppContext {
    FTDevice device;
    DeviceMode mode = DeviceMode::None;

    bool isConnected() const { return device.isOpen(); }

    void reset() { mode = DeviceMode::None; }
};
