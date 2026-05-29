#pragma once

#include <cstdint>

// Portable types matching LibFT4222 / D2XX names (no proprietary headers required).

using FT_STATUS = uint32_t;
inline constexpr FT_STATUS FT_OK = 0;

using FT4222_STATUS = uint32_t;
inline constexpr FT4222_STATUS FT4222_OK = 0;

enum FT4222_SPIMode : int {
    SPI_IO_SINGLE = 0,
    SPI_IO_DUAL = 1,
    SPI_IO_QUAD = 2,
};

enum FT4222_SPICPOL : int {
    CLK_IDLE_LOW = 0,
    CLK_IDLE_HIGH = 1,
};

enum FT4222_SPICPHA : int {
    CLK_LEADING = 0,
    CLK_TRAILING = 1,
};

enum FT4222_SPIClock : int {
    CLK_DIV_2 = 0,
    CLK_DIV_4 = 1,
    CLK_DIV_8 = 2,
    CLK_DIV_16 = 3,
    CLK_DIV_32 = 4,
    CLK_DIV_64 = 5,
    CLK_DIV_128 = 6,
    CLK_DIV_256 = 7,
    CLK_DIV_512 = 8,
};

enum FT4222_ClockRate : int {
    SYS_CLK_24 = 0,
    SYS_CLK_48 = 1,
    SYS_CLK_60 = 2,
    SYS_CLK_80 = 3,
};

enum GPIO_Dir : int {
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1,
};

enum GPIO_Port : int {
    GPIO_PORT0 = 0,
    GPIO_PORT1 = 1,
    GPIO_PORT2 = 2,
    GPIO_PORT3 = 3,
};
