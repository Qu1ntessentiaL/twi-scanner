#include "CommandRouter.hpp"

#include <iostream>

void CommandRouter::registerCommand(const std::string &name, CommandFn fn, const std::string &desc) {
    m_commands[name] = CommandEntry{std::move(fn), desc};
}

bool CommandRouter::execute(AppContext &ctx, const std::string &line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd.empty())
        return true;

    const auto it = m_commands.find(cmd);
    if (it == m_commands.end()) {
        std::cout << "Unknown command: " << cmd << "\n";
        return false;
    }

    it->second.fn(ctx, iss);
    return true;
}

std::vector<std::pair<std::string, std::string>> CommandRouter::listCommands() const {
    std::vector<std::pair<std::string, std::string>> res;
    res.reserve(m_commands.size());
    for (const auto &p : m_commands)
        res.emplace_back(p.first, p.second.desc);
    return res;
}

const std::string *CommandRouter::findDescription(const std::string &name) const {
    const auto it = m_commands.find(name);
    if (it == m_commands.end())
        return nullptr;
    return &it->second.desc;
}
