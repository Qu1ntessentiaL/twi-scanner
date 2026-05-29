#include "ParseUtil.hpp"

#include <cctype>
#include <stdexcept>

unsigned long parseNumber(const std::string &s) {
    if (s.empty()) throw std::invalid_argument("empty");
    bool hasHexChar = false;
    for (char c : s) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            hasHexChar = true;
            break;
        }
    }
    const int base = hasHexChar || (s.size() > 1 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
                         ? 16
                         : 10;
    return std::stoul(s, nullptr, base);
}
