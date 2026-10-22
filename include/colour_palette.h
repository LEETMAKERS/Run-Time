#pragma once
#include <stdint.h>

namespace game {
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
// Standard RGB colors in RGB565.
constexpr uint16_t buttonColours[] = {
    rgb565(255, 0, 0),//red
    rgb565(0, 0, 255),//blue
    0x1200, //green
    0x59a0,  //brown
};
}
