#include "Commands.hpp"
#include "cli/ParseUtil.hpp"
#include "ft4222/ft4222.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

static bool requireConnection(const AppContext &ctx) {
    if (!ctx.isConnected()) {
        cout << "Not connected. Use 'connect <index>' or 'connect --serial <sn>' first.\n";
        return false;
    }
    return true;
}

void registerDeviceCommands(CommandRouter &router) {
    // list devices
    router.registerCommand("devices",
        [](AppContext &, istringstream &) {
            DeviceEnumerator::printDevices();
        },
        "List FT4222 devices");

    router.registerCommand("connect",
        [](AppContext &ctx, istringstream &iss) {
            string arg1;
            if (!(iss >> arg1)) {
                cout << "Usage: connect <index> | connect --serial <sn> | connect s:<sn>\n";
                return;
            }

            auto connectSerial = [&](const string &serial) {
                try {
                    ctx.device.openBySerial(serial);
                    ctx.mode = DeviceMode::None;
                    cout << "Connected to device serial " << serial << "\n";
                } catch (const exception &ex) {
                    cout << "Failed to connect: " << ex.what() << "\n";
                }
            };

            if (arg1 == "--serial" || arg1 == "serial") {
                string serial;
                if (!(iss >> serial)) {
                    cout << "Usage: connect --serial <serial>\n";
                    return;
                }
                connectSerial(serial);
                return;
            }

            if (arg1.size() >= 2 && arg1.compare(0, 2, "s:") == 0) {
                connectSerial(arg1.substr(2));
                return;
            }

            try {
                const unsigned long idx = parseNumber(arg1);
                ctx.device.open(static_cast<uint32_t>(idx));
                ctx.mode = DeviceMode::None;
                cout << "Connected to device index " << idx << "\n";
            } catch (const exception &ex) {
                cout << "Failed to connect: " << ex.what() << "\n";
            }
        },
        "Connect: connect <index> | connect --serial <sn> | connect s:<sn>");

    router.registerCommand("disconnect",
        [](AppContext &ctx, istringstream &) {
            try {
                ctx.device.close();
            } catch (const exception &ex) {
                cerr << "disconnect warning: " << ex.what() << "\n";
            }
            ctx.reset();
            cout << "Disconnected\n";
        },
        "Disconnect from device");

    router.registerCommand("status",
        [](AppContext &ctx, istringstream &) {
            cout << "Connected: " << (ctx.isConnected() ? "yes" : "no") << "\n";
            if (!ctx.isConnected()) return;
            try {
                cout << "Version: " << ctx.device.getVersionString() << "\n";
            } catch (const exception &e) {
                cout << "Version: (error) " << e.what() << "\n";
            }

            try {
                auto mode = ctx.device.getDeviceMode();
                string s;
                switch (mode) {
                    case FTDevice::Mode::I2C_Master: s = "I2C"; break;
                    case FTDevice::Mode::SPI_Master: s = "SPI"; break;
                    case FTDevice::Mode::GPIO: s = "GPIO"; break;
                    default: s = "Unknown"; break;
                }
                cout << "Device mode: " << s << "\n";
            } catch (const exception &e) {
                cout << "Device mode: (error) " << e.what() << "\n";
            }
        },
        "Show device connection status");

    // I2C init
    router.registerCommand("i2c_init",
        [](AppContext &ctx, istringstream &iss) {
            if (!requireConnection(ctx)) return;
            string speedStr;
            if (!(iss >> speedStr)) {
                // default
                speedStr = "400";
            }
            try {
                unsigned long s = parseNumber(speedStr);
                FTDevice::I2CSpeed speed = FTDevice::I2CSpeed::S400K;
                if (s == 100) speed = FTDevice::I2CSpeed::S100K;
                else if (s == 400) speed = FTDevice::I2CSpeed::S400K;
                else if (s == 1000) speed = FTDevice::I2CSpeed::S1M;
                else {
                    cout << "Unsupported speed, use 100, 400, 1000\n";
                    return;
                }

                ctx.device.initI2CMaster(speed);
                ctx.mode = DeviceMode::I2C;
                cout << "I2C initialized at " << s << " kbps\n";
            } catch (const exception &ex) {
                cout << "i2c_init failed: " << ex.what() << "\n";
            }
        },
        "Initialize I2C master [speed: 100|400|1000]");

    // i2c_send <addr> <bytes...>
    router.registerCommand("i2c_send",
        [](AppContext &ctx, istringstream &iss) {
            if (!requireConnection(ctx)) return;
            string addrStr;
            if (!(iss >> addrStr)) { cout << "Usage: i2c_send <addr> <hex-bytes...>\n"; return; }
            vector<unsigned long> bytes;
            string token;
            while (iss >> token) {
                try {
                    bytes.push_back(parseNumber(token));
                } catch (const exception &e) {
                    cout << "Invalid byte: " << token << "\n"; return;
                }
            }
            if (bytes.empty()) { cout << "No data to send\n"; return; }

            try {
                uint8_t addr = static_cast<uint8_t>(parseNumber(addrStr));
                vector<uint8_t> data;
                data.reserve(bytes.size());
                for (auto b : bytes) data.push_back(static_cast<uint8_t>(b & 0xFF));
                ctx.device.i2cMasterWrite(addr, data);
                cout << "Wrote " << data.size() << " bytes to 0x" << hex << (int)addr << dec << "\n";
            } catch (const exception &ex) {
                cout << "i2c_send failed: " << ex.what() << "\n";
            }
        },
        "i2c_send <addr> <hex bytes...>  - send data to I2C device");

    // i2c_recv <addr> <count>
    router.registerCommand("i2c_recv",
        [](AppContext &ctx, istringstream &iss) {
            if (!requireConnection(ctx)) return;
            string addrStr; size_t count = 0;
            if (!(iss >> addrStr >> count)) { cout << "Usage: i2c_recv <addr> <count>\n"; return; }
            try {
                uint8_t addr = static_cast<uint8_t>(parseNumber(addrStr));
                auto data = ctx.device.i2cMasterRead(addr, count);
                cout << "Read " << data.size() << " bytes: ";
                cout << hex;
                for (auto b : data) cout << uppercase << setw(2) << setfill('0') << (int)b << " ";
                cout << dec << setfill(' ') << "\n";
            } catch (const exception &ex) {
                cout << "i2c_recv failed: " << ex.what() << "\n";
            }
        },
        "i2c_recv <addr> <count> - read <count> bytes from I2C device");

    // i2c_scan [start] [end]
    router.registerCommand("i2c_scan",
        [](AppContext &ctx, istringstream &iss) {
            if (!requireConnection(ctx)) return;
            string sstart, send;
            uint8_t start = 0x03, end = 0x77;
            if (iss >> sstart) start = static_cast<uint8_t>(parseNumber(sstart));
            if (iss >> send) end = static_cast<uint8_t>(parseNumber(send));
            try {
                auto found = ctx.device.scanI2CBus(start, end);
                if (found.empty()) { cout << "No devices found\n"; return; }
                cout << "Found devices: ";
                cout << hex;
                for (auto a : found) cout << "0x" << setw(2) << setfill('0') << (int)a << " ";
                cout << dec << setfill(' ') << "\n";
            } catch (const exception &ex) {
                cout << "i2c_scan failed: " << ex.what() << "\n";
            }
        },
        "i2c_scan [start] [end] - scan I2C bus for devices");

    router.registerCommand("i2c_status",
        [](AppContext &ctx, istringstream &) {
            if (!requireConnection(ctx)) return;
            try {
                uint8_t st = ctx.device.i2cMasterGetStatus();
                cout << "I2C controller status: 0x" << hex << (int)st << dec << "\n";
            } catch (const exception &ex) { cout << "i2c_status failed: " << ex.what() << "\n"; }
        },
        "Show I2C controller status");

    router.registerCommand("i2c_reset",
        [](AppContext &ctx, istringstream &) {
            if (!requireConnection(ctx)) return;
            try {
                ctx.device.i2cMasterResetBus();
                cout << "I2C bus reset\n";
            } catch (const exception &ex) { cout << "i2c_reset failed: " << ex.what() << "\n"; }
        },
        "Reset I2C bus");
    // SPI commands
    router.registerCommand("spi_init",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            std::string modeStr = "single", clkStr = "512", polStr = "low", phaseStr = "leading";
            iss >> modeStr >> clkStr >> polStr >> phaseStr;
            try {
                FT4222_SPIMode mode = SPI_IO_SINGLE;
                if (!modeStr.empty()) {
                    char c = std::tolower(static_cast<unsigned char>(modeStr[0]));
                    if (c == 'd') mode = SPI_IO_DUAL;
                    else if (c == 'q') mode = SPI_IO_QUAD;
                }

                unsigned long cd = parseNumber(clkStr);
                FTDevice::SPIClockDivider div = FTDevice::SPIClockDivider::DIV_512;
                switch (cd) {
                    case 2: div = FTDevice::SPIClockDivider::DIV_2; break;
                    case 4: div = FTDevice::SPIClockDivider::DIV_4; break;
                    case 8: div = FTDevice::SPIClockDivider::DIV_8; break;
                    case 16: div = FTDevice::SPIClockDivider::DIV_16; break;
                    case 32: div = FTDevice::SPIClockDivider::DIV_32; break;
                    case 64: div = FTDevice::SPIClockDivider::DIV_64; break;
                    case 128: div = FTDevice::SPIClockDivider::DIV_128; break;
                    case 256: div = FTDevice::SPIClockDivider::DIV_256; break;
                    case 512: div = FTDevice::SPIClockDivider::DIV_512; break;
                    default: cout << "Unsupported clock divider, use 2/4/8/16/32/64/128/256/512\n"; return;
                }

                FT4222_SPICPOL pol = CLK_IDLE_LOW;
                if (!polStr.empty() && std::tolower(static_cast<unsigned char>(polStr[0])) == 'h') pol = CLK_IDLE_HIGH;

                FT4222_SPICPHA pha = CLK_LEADING;
                if (!phaseStr.empty() && (std::tolower(static_cast<unsigned char>(phaseStr[0])) == 't' || phaseStr[0] == '1')) pha = CLK_TRAILING;

                ctx.device.initSPIMaster(mode, div, pol, pha);
                ctx.mode = DeviceMode::SPI;
                cout << "SPI initialized: mode=" << modeStr << " clkDiv=" << cd << "\n";
            } catch (const exception &ex) {
                cout << "spi_init failed: " << ex.what() << "\n";
            }
        },
        "Initialize SPI master [mode: single|dual|quad] [clkDiv: 2..512] [pol: low|high] [phase: leading|trailing]");

    router.registerCommand("spi_send",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            std::vector<unsigned long> bytes;
            std::string token;
            while (iss >> token) {
                try { bytes.push_back(parseNumber(token)); } catch (const exception &) { cout << "Invalid byte: " << token << "\n"; return; }
            }
            if (bytes.empty()) { cout << "Usage: spi_send <hex-bytes...>\n"; return; }
            try {
                std::vector<uint8_t> data; data.reserve(bytes.size()); for (auto b : bytes) data.push_back(static_cast<uint8_t>(b & 0xFF));
                ctx.device.spiMasterSingleWrite(data);
                cout << "SPI wrote " << data.size() << " bytes\n";
            } catch (const exception &ex) { cout << "spi_send failed: " << ex.what() << "\n"; }
        },
        "spi_send <hex-bytes...> - write data over SPI");

    router.registerCommand("spi_recv",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            size_t count = 0;
            if (!(iss >> count)) { cout << "Usage: spi_recv <count>\n"; return; }
            try {
                auto data = ctx.device.spiMasterSingleRead(count);
                cout << "Read " << data.size() << " bytes: ";
                cout << std::hex;
                for (auto b : data) cout << std::uppercase << std::setw(2) << std::setfill('0') << (int)b << " ";
                cout << std::dec << std::setfill(' ') << "\n";
            } catch (const exception &ex) { cout << "spi_recv failed: " << ex.what() << "\n"; }
        },
        "spi_recv <count> - read <count> bytes from SPI");

    router.registerCommand("spi_xfer",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            std::vector<unsigned long> bytes; std::string token;
            while (iss >> token) { try { bytes.push_back(parseNumber(token)); } catch (const exception &) { cout << "Invalid byte: " << token << "\n"; return; } }
            if (bytes.empty()) { cout << "Usage: spi_xfer <hex-bytes...>\n"; return; }
            try {
                std::vector<uint8_t> data; data.reserve(bytes.size()); for (auto b : bytes) data.push_back(static_cast<uint8_t>(b & 0xFF));
                auto out = ctx.device.spiMasterSingleReadWrite(data);
                cout << "Received " << out.size() << " bytes: ";
                cout << std::hex;
                for (auto b : out) cout << std::uppercase << std::setw(2) << std::setfill('0') << (int)b << " ";
                cout << std::dec << std::setfill(' ') << "\n";
            } catch (const exception &ex) { cout << "spi_xfer failed: " << ex.what() << "\n"; }
        },
        "spi_xfer <hex-bytes...> - write and read simultaneously over SPI");

    router.registerCommand("spi_status",
        [](AppContext &ctx, std::istringstream &) {
            if (!requireConnection(ctx)) return;
            try {
                auto mode = ctx.device.getDeviceMode();
                std::string s;
                switch (mode) {
                    case FTDevice::Mode::I2C_Master: s = "I2C"; break;
                    case FTDevice::Mode::SPI_Master: s = "SPI"; break;
                    case FTDevice::Mode::GPIO: s = "GPIO"; break;
                    default: s = "Unknown"; break;
                }
                cout << "Device mode: " << s << "\n";
                cout << "Version: " << ctx.device.getVersionString() << "\n";
            } catch (const exception &ex) { cout << "spi_status failed: " << ex.what() << "\n"; }
        },
        "Show SPI/device status");

    // GPIO commands
    router.registerCommand("gpio_init",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            auto parseDir = [](const std::string &s){
                if (s.empty()) return GPIO_INPUT;
                char c = std::tolower(static_cast<unsigned char>(s[0]));
                return (c == 'o' || c == '1') ? GPIO_OUTPUT : GPIO_INPUT;
            };
            std::string d0,d1,d2,d3;
            iss >> d0 >> d1 >> d2 >> d3;
            try {
                ctx.device.initGPIO(parseDir(d0), parseDir(d1), parseDir(d2), parseDir(d3));
                ctx.mode = DeviceMode::GPIO;
                cout << "GPIO initialized\n";
            } catch (const exception &ex) { cout << "gpio_init failed: " << ex.what() << "\n"; }
        },
        "gpio_init [d0 d1 d2 d3] - each: in/out (default in)");

    router.registerCommand("gpio_read",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            int port = 0; if (!(iss >> port)) { cout << "Usage: gpio_read <port 0-3>\n"; return; }
            try { bool v = ctx.device.readGPIO(static_cast<GPIO_Port>(port)); cout << "GPIO" << port << " = " << (v?"1":"0") << "\n"; } catch (const exception &ex) { cout << "gpio_read failed: " << ex.what() << "\n"; }
        },
        "gpio_read <port> - read GPIO port (0-3)");

    router.registerCommand("gpio_write",
        [](AppContext &ctx, std::istringstream &iss) {
            if (!requireConnection(ctx)) return;
            int port; int val; if (!(iss >> port >> val)) { cout << "Usage: gpio_write <port 0-3> <0|1>\n"; return; }
            try { ctx.device.writeGPIO(static_cast<GPIO_Port>(port), val?true:false); cout << "GPIO" << port << " set to " << val << "\n"; } catch (const exception &ex) { cout << "gpio_write failed: " << ex.what() << "\n"; }
        },
        "gpio_write <port> <0|1> - set GPIO port (0-3)");

    router.registerCommand("gpio_status",
        [](AppContext &ctx, std::istringstream &) {
            if (!requireConnection(ctx)) return;
            try {
                cout << "GPIO states: ";
                for (int p = 0; p < 4; ++p) {
                    bool v = ctx.device.readGPIO(static_cast<GPIO_Port>(p));
                    cout << "P" << p << ":" << (v?"1":"0") << " ";
                }
                cout << "\n";
            } catch (const exception &ex) { cout << "gpio_status failed: " << ex.what() << "\n"; }
        },
        "gpio_status - read all gpio ports");
}

