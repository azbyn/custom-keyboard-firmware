#pragma once
#include <stdint.h>
#include "hardware/spi.h"

using Pin = unsigned;

namespace pins {

constexpr Pin unused_pins[] = {
    26, 27, 
    41, 42, 43, 44, 45
};
namespace disp {

//18 SPI0 SCK
//19 SPI0 TX
//20 SPI0 RX
//21 SPI0 CSn
//22 SPI0 SCK

constexpr Pin cs  = 20;//8
constexpr Pin scl = 18;//6
constexpr Pin sda = 19;//5
constexpr Pin dc  = 21;//4 //aka rs
constexpr Pin rst = 22;//3
constexpr int bl = 35;
spi_inst_t* const spiPtr = spi0;

};

//namespace kbd {
constexpr Pin rowPins[] = {
    33,//32,
    31, 
    30, 29,
    32, 
    // 25
    36
};
constexpr Pin colPins[] = {
    0, 1, 2, 3, 4, 
    5, 6, 7, 8, 9, 
    10, 11, 12, 13, 14, 
    15, 16, 17,
};

constexpr size_t colLen = sizeof(colPins) / sizeof(colPins[0]);
constexpr size_t rowLen = sizeof(rowPins) / sizeof(rowPins[0]);

//};

constexpr Pin encoder_push = 34;
constexpr Pin encoder_A = 23;
constexpr Pin encoder_B = 24;

constexpr Pin bumper = 40;

constexpr Pin ledRuski = 38;
constexpr Pin ledCompose = 39;
constexpr Pin ledMenu = 37;//idk what else to do with it yet




// constexpr Pin leds[] = {
//     37, 38, 39
//     // 36, 37,
//     // 35, 47, 48
// };

// constexpr Pin battery = 40;

namespace wifi {
// 47 SPI1 TX
// 46 SPI1 CLK
// 40/28 SPI1 RX

// constexpr Pin ce  = 35;
// constexpr Pin cs  = 36;


constexpr Pin miso = 28; 
constexpr Pin mosi = 47;
constexpr Pin clk  = 46;
//constexpr Pin int = 22;
constexpr int intr = -1;
spi_inst_t* const spiPtr = spi1;

};

//35, 35, 37, 38, 39

// constexpr Pin uart = 33;

//remaining usable pins: 40, 47, 46
};