#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "context/AppContext.hpp"

using CommandFn = std::function<void(AppContext &, std::istringstream &)>;

class CommandRouter {
public:
    void registerCommand(const std::string &name, CommandFn fn, const std::string &desc = "");
    bool execute(AppContext &ctx, const std::string &line);
    std::vector<std::pair<std::string, std::string>> listCommands() const;
    const std::string *findDescription(const std::string &name) const;

private:
    struct CommandEntry {
        CommandFn fn;
        std::string desc;
    };

    std::unordered_map<std::string, CommandEntry> m_commands;
};
