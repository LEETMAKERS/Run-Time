#include "include/hardware.h"
#include "include/display.h"

namespace {
hardware::Inputs inputs;
uint8_t previousHeld = 0;
uint8_t pressedOnce = 0;
uint8_t checked = 0;
uint8_t modes = 0;
bool previousMode = false;
bool firstFrame = true;
}

void setup() {
  gameDisplay.begin(hardware::baudRate);
  inputs.begin();
  gameDisplay.println(F("HARDWARE TEST: press and release every button; flip MODE both ways."));
  gameDisplay.println(F("Check color bars and the white border on all four screen edges."));
}

void loop() {
  inputs.poll();
  uint8_t held = 0;
  for (uint8_t i = 0; i < game::colourCount; ++i) {
    if (inputs.colours[i].down()) held |= 1 << i;
  }
  if (inputs.start.down()) held |= 1 << 4;
  pressedOnce |= held;
  // A control is checked only after a complete press and release.
  checked |= previousHeld & ~held & pressedOnce;
  const bool speed = inputs.mode.down();
  modes |= speed ? 1 : 2;
  if (firstFrame || held != previousHeld || speed != previousMode) {
    SCREEN(diagnostic(held, checked, speed, modes));
    const char* names[] = {"RED", "BLUE", "GREEN", "BROWN", "START"};
    for (uint8_t i = 0; i < 5; ++i) {
      gameDisplay.print(names[i]);
      gameDisplay.print(held & (1 << i) ? F(": DOWN") : F(": UP"));
      gameDisplay.println(checked & (1 << i) ? F(" OK") : F(" --"));
    }
    gameDisplay.println(speed ? F("MODE: SPEED") : F("MODE: MEMORY"));
    if (checked == 0x1F && modes == 3) {
      gameDisplay.println(F("All controls checked. Verify screen colors and border visually."));
    }
    previousHeld = held;
    previousMode = speed;
    firstFrame = false;
  }
  delay(1);
}
