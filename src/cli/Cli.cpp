#include "Cli.hpp"
#include "Commands.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef HAVE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#include <cstdlib>
#endif

#ifndef TWI_SCANNER_VERSION
#define TWI_SCANNER_VERSION "0.1"
#endif

int Cli::run(int argc, char *argv[]) {
    registerCommands();

    std::vector<std::string> batchCommands;
    bool interactive = true;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--version" || arg == "-V") {
            std::cout << "twi-scanner " << TWI_SCANNER_VERSION << "\n";
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "-c" || arg == "--command") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
            batchCommands.push_back(argv[++i]);
            interactive = false;
            continue;
        }
        std::cerr << "Unknown option: " << arg << "\n";
        printUsage(argv[0]);
        return 1;
    }

    if (!batchCommands.empty()) {
        return runBatch(batchCommands) ? 0 : 1;
    }

    runInteractive();
    return 0;
}

bool Cli::runBatch(const std::vector<std::string> &commands) {
    for (const auto &line : commands) {
        if (!m_router.execute(m_ctx, line))
            return false;
        if (!m_running)
            break;
    }
    return true;
}

void Cli::runInteractive() {
    std::cout << "FT4222 CLI. Type 'help'\n";
#ifdef TWI_MOCK_FT4222
    std::cout << "Note: built in mock mode (no LibFT4222). Hardware I/O is simulated.\n";
#endif

#ifdef HAVE_READLINE
    const char *home = std::getenv("HOME");
    std::string historyPath =
        home ? std::string(home) + "/.twi-scanner_history" : ".twi-scanner_history";
    read_history(historyPath.c_str());

    while (m_running) {
        std::string pr = prompt() + " >> ";
        char *input = readline(pr.c_str());
        if (!input)
            break;

        std::string line(input);
        if (!line.empty()) {
            add_history(input);
            write_history(historyPath.c_str());
        }

        m_router.execute(m_ctx, line);
        free(input);
    }

    write_history(historyPath.c_str());
#else
    while (m_running) {
        std::cout << prompt() << " >> ";

        std::string line;
        if (!std::getline(std::cin, line))
            break;

        m_router.execute(m_ctx, line);
    }
#endif
}

void Cli::registerCommands() {
    auto doExit = [this](AppContext &, std::istringstream &) { m_running = false; };
    m_router.registerCommand("exit", doExit, "Exit the CLI");
    m_router.registerCommand("quit", doExit, "Alias for exit");

    m_router.registerCommand(
        "clear",
        [](AppContext &, std::istringstream &) {
            std::cout << "\033[2J\033[H" << std::flush;
        },
        "Clear the screen");

    m_router.registerCommand(
        "help",
        [this](AppContext &, std::istringstream &iss) {
            std::string topic;
            iss >> topic;
            if (!topic.empty()) {
                const std::string *desc = m_router.findDescription(topic);
                if (!desc) {
                    std::cout << "Unknown command: " << topic << "\n";
                    return;
                }
                std::cout << topic;
                if (!desc->empty())
                    std::cout << " - " << *desc;
                std::cout << "\n";
                return;
            }

            auto cmds = m_router.listCommands();
            std::cout << "Available commands:\n";
            std::sort(cmds.begin(), cmds.end(),
                      [](const auto &a, const auto &b) { return a.first < b.first; });
            for (const auto &c : cmds) {
                std::cout << "  " << c.first;
                if (!c.second.empty())
                    std::cout << " - " << c.second;
                std::cout << "\n";
            }
        },
        "Show help (usage: help [command])");

    registerDeviceCommands(m_router);
}

std::string Cli::prompt() const {
    if (!m_ctx.isConnected())
        return "ft4222";

    switch (m_ctx.mode) {
    case DeviceMode::I2C:
        return "ft4222[i2c]";
    case DeviceMode::SPI:
        return "ft4222[spi]";
    case DeviceMode::GPIO:
        return "ft4222[gpio]";
    default:
        return "ft4222";
    }
}

void Cli::printUsage(const char *prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "  -h, --help           Show this help\n"
              << "  -V, --version        Show version\n"
              << "  -c, --command <cmd>  Run command and exit (repeatable)\n"
              << "\nExamples:\n"
              << "  " << prog << " -c devices\n"
              << "  " << prog << " -c \"connect 0\" -c \"i2c_init\" -c \"i2c_scan\"\n";
}
