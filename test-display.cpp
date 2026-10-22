#include "include/hardware.h"
#include "include/colour_palette.h"
#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ST7789.h>

namespace {
Adafruit_ILI9341 ili(5, 16, 4);
Adafruit_ST7789 st(5, 16, 4);
hardware::Inputs inputs;
uint8_t profile = 0;

void drawProfile() {
  const bool useST = profile >= 4;
  const bool portrait = (profile & 2) != 0;
  const bool swap = (profile & 1) != 0;
  Adafruit_SPITFT* panel;
  if (useST) {
    st.init(240, 320);
    st.setSPISpeed(4000000);
    st.invertDisplay(false);
    st.setRotation(portrait ? 0 : 1);
    panel = &st;
  } else {
    ili.begin(4000000);
    ili.setRotation(portrait ? 0 : 1);
    ili.invertDisplay(false);
    panel = &ili;
    Serial.print("ILI ID bytes: ");
    for (uint8_t i = 1; i <= 3; ++i) {
      Serial.print(ili.readcommand8(0xD3, i), HEX);
      Serial.print(' ');
    }
    Serial.println();
  }
  // Override color order at the controller so white stays white.
  // ILI9341 native BGR; ST7789 native RGB.
  uint8_t madctl = useST ? (portrait ? 0xC0 : 0xA0)
                        : (portrait ? 0x48 : 0x28);
  if (swap) madctl ^= 0x08;
  panel->sendCommand(0x36, &madctl, 1);
  const uint8_t pixelFormat = 0x55; // 16-bit RGB565.
  panel->sendCommand(0x3A, &pixelFormat, 1);
  panel->fillScreen(ILI9341_BLACK);
  const int w = panel->width(), h = panel->height();
  panel->setTextWrap(false);
  panel->setTextColor(ILI9341_WHITE);
  panel->setTextSize(2);
  panel->setCursor(8, 10);
  panel->print("TEST "); panel->print(profile + 1);
  panel->setTextSize(1);
  panel->setCursor(8, 34);
  panel->print(useST ? "ST7789" : "ILI9341");
  panel->print(portrait ? " portrait" : " landscape");
  panel->print(swap ? " swap" : " native");
  const uint16_t colors[] = {game::buttonColours[0], game::buttonColours[1], game::buttonColours[2], game::buttonColours[3], 0xFFFF};
  const char* names[] = {"RED", "BLUE", "GREEN", "BROWN", "WHITE"};
  for (int i = 0; i < 5; ++i) {
    const int top = 54 + i * (h - 90) / 5;
    const int bottom = 54 + (i + 1) * (h - 90) / 5;
    panel->fillRect(4, top, w - 8, bottom - top, colors[i]);
    panel->setTextColor((i == 2 || i == 4) ? ILI9341_BLACK : ILI9341_WHITE);
    panel->setCursor(12, top + 8);
    panel->print(names[i]);
  }
  panel->setTextColor(ILI9341_WHITE);
  panel->setCursor(8, h - 26);
  panel->print("START: next test (1-8)");
  panel->drawRect(0, 0, w, h, ILI9341_WHITE);
  panel->fillRect(w - 10, h - 10, 10, 10, ILI9341_WHITE);
  Serial.print("TEST "); Serial.print(profile + 1);
  Serial.print(useST ? ": ST7789" : ": ILI9341");
  Serial.print(portrait ? " portrait" : " landscape");
  Serial.println(swap ? " swapped color order" : " native color order");
}
}

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, 5);
  inputs.begin();
  drawProfile();
}

void loop() {
  inputs.poll();
  if (inputs.start.pressed) {
    profile = (profile + 1) % 8;
    drawProfile();
  }
  delay(1);
}
