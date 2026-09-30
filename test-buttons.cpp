#include "include/hardware.h"

namespace {
hardware::Inputs inputs;
void showMode() {
  Serial.println(inputs.mode.down() ? F("MODE: SPEED SPRINT") : F("MODE: MEMORY QUEST"));
}
}

void setup() {
  Serial.begin(hardware::baudRate);
  inputs.begin();
  Serial.println(F("INPUT TEST - press and release each button. Flip MODE."));
  showMode();
}

void loop() {
  inputs.poll();
  for (uint8_t i = 0; i < game::colourCount; ++i) {
    if (inputs.colours[i].pressed || inputs.colours[i].released) {
      Serial.print(hardware::colourName(i));
      Serial.println(inputs.colours[i].down() ? F(" PRESSED") : F(" RELEASED"));
    }
  }
  if (inputs.start.pressed) Serial.println(F("START PRESSED"));
  if (inputs.start.released) Serial.println(F("START RELEASED"));
  if (inputs.mode.pressed || inputs.mode.released) showMode();
  delay(1);
}
