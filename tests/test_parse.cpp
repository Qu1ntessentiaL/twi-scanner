#include "cli/ParseUtil.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

static void testDecimal() {
    assert(parseNumber("42") == 42);
    assert(parseNumber("0") == 0);
}

static void testHex() {
    assert(parseNumber("0x10") == 16);
    assert(parseNumber("ff") == 255);
    assert(parseNumber("0X2A") == 42);
}

static void testEmptyThrows() {
    bool threw = false;
    try {
        parseNumber("");
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    assert(threw);
}

int main() {
    testDecimal();
    testHex();
    testEmptyThrows();
    std::cout << "test_parse: OK\n";
    return 0;
}
