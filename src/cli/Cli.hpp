#pragma once

#include "CommandRouter.hpp"

#include <string>
#include <vector>

class Cli {
public:
    int run(int argc, char *argv[]);

private:
    AppContext m_ctx;
    CommandRouter m_router;
    bool m_running = true;

    void registerCommands();
    std::string prompt() const;
    bool runBatch(const std::vector<std::string> &commands);
    void runInteractive();
    static void printUsage(const char *prog);
};
