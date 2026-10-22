#pragma once

#include <Arduino.h>
#include "game_rules.h"

namespace hardware {
constexpr uint8_t colourPins[] = {25, 26, 27, 32};
constexpr uint8_t startPin = 22;
constexpr uint8_t modePin = 21;
constexpr unsigned long baudRate = 115200;

inline const __FlashStringHelper* colourName(uint8_t colour) {
  switch (colour) {
    case 0: return F("RED");
    case 1: return F("BLUE");
    case 2: return F("GREEN");
    case 3: return F("BROWN");
    default: return F("UNKNOWN");
  }
}

class Inputs {
 public:
  game::Button colours[game::colourCount];
  game::Button start;
  game::Button mode;
  game::ColourGate gate;
  int8_t answer = -1;

  void begin() {
    for (uint8_t i = 0; i < game::colourCount; ++i) {
      pinMode(colourPins[i], INPUT_PULLUP);
      colours[i].begin(digitalRead(colourPins[i]) == LOW, millis());
    }
    pinMode(startPin, INPUT_PULLUP);
    pinMode(modePin, INPUT_PULLUP);
    start.begin(digitalRead(startPin) == LOW, millis());
    mode.begin(digitalRead(modePin) == LOW, millis());
  }
  void poll() {
    const uint32_t now = millis();
    uint8_t mask = 0;
    for (uint8_t i = 0; i < game::colourCount; ++i) {
      colours[i].update(digitalRead(colourPins[i]) == LOW, now);
      if (colours[i].down()) mask |= 1 << i;
    }
    start.update(digitalRead(startPin) == LOW, now);
    mode.update(digitalRead(modePin) == LOW, now);
    answer = gate.update(mask);
  }
};
}  // namespace hardware
