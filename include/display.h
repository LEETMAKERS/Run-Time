#pragma once

#include "hardware.h"
#include "colour_palette.h"

#ifdef ARDUINO_ARCH_ESP32
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#ifndef TFT_INVERT_COLORS
#define TFT_INVERT_COLORS 0
#endif
#ifndef TFT_SWAP_RED_BLUE
#define TFT_SWAP_RED_BLUE 0
#endif

namespace hardware {
// Standard ESP32 VSPI: SCK=18, MOSI=23, MISO=19.
class GameDisplay : public Print {
 public:
  void begin(unsigned long baud) {
    Serial.begin(baud);
    SPI.begin(18, 19, 23, 5);
    // Match the hardware-verified diagnostic TEST 5 exactly.
    tft_.init(240, 320);
    tft_.setSPISpeed(4000000);
    tft_.setRotation(1);
    tft_.setTextWrap(false);
    tft_.invertDisplay(TFT_INVERT_COLORS);
    clear();
  }
  void clear() {
    tft_.fillScreen(ST77XX_BLACK);
    tft_.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft_.setTextSize(2);
    tft_.setCursor(0, 0);
  }
  // Serial logs never draw onto the TFT; each view owns its layout.
  size_t write(uint8_t value) override { return Serial.write(value); }
  void flush() { Serial.flush(); }
  void title(const char* heading, const char* hint = "START: menu") {
    clear();
    tft_.fillRect(0, 0, tft_.width(), sy(40), ST77XX_BLACK);
    text(12, 12, heading, 2);
    text(12, 222, hint, 1, 0xBDF7);
  }
  void text(int x, int y, const String& value, uint8_t size = 2,
            uint16_t color = ST77XX_WHITE) {
    tft_.setTextSize(size);
    tft_.setTextColor(color);
    tft_.setCursor(sx(x), sy(y));
    tft_.print(value);
  }
  void colour(uint8_t index, bool label = true) {
    uint16_t color = game::buttonColours[index];
#if TFT_SWAP_RED_BLUE
    color = ((color & 0x001F) << 11) | (color & 0x07E0) | ((color >> 11) & 0x001F);
#endif
    tft_.fillScreen(color);
    if (!label) return;
    const char* names[] = {"RED", "BLUE", "GREEN", "BROWN"};
    tft_.setTextSize(3);
    tft_.setTextColor(ST77XX_WHITE);
    tft_.setCursor((tft_.width() - strlen(names[index]) * 18) / 2,
                   (tft_.height() - 24) / 2);
    tft_.print(names[index]);
  }
  void menu(game::Mode selected, uint16_t sprintBest, uint32_t memoryBest,
            uint32_t wordBest, bool hacked = false) {
    lossAnimation_ = false;
    clear();
    tft_.fillRect(0, 0, tft_.width(), sy(42), 0x1082);
    text(12, 8, "RUN TIME", 3, 0x07FF);
    text(205, 12, "ARCADE", 2, 0xFFE0);
    const uint32_t target = selected == game::Mode::Memory ? memoryBest :
                            selected == game::Mode::Speed ? sprintBest : wordBest;
    tft_.fillRoundRect(sx(8), sy(47), sx(304), sy(36), 6, 0x2124);
    text(16, 51, hacked ? "HACK MODE - NO RECORDS" : "RECORD TO BEAT", 1, 0xFFE0);
    text(16, 68, selected == game::Mode::Memory ? "LEVELS CLEARED" : "TOTAL POINTS", 1, 0xBDF7);
    text(232, 51, String(target), 3, 0xFFE0);
    const char* names[] = {"MEMORY QUEST", "SPEED SPRINT", "WORD RUSH"};
    const uint16_t accents[] = {0x07FF, 0xFFE0, 0xF81F};
    const String details[] = {String("Watch, then repeat | 10 levels"),
                              String("Match the color | 15 seconds"),
                              String("Ignore the background | 15 sec")};
    for (uint8_t i = 0; i < 3; ++i) {
      const int y = 90 + i * 40;
      const bool active = i == static_cast<uint8_t>(selected);
      tft_.fillRoundRect(sx(8), sy(y), sx(304), sy(35), 7, active ? 0x2124 : 0x1082);
      tft_.fillRoundRect(sx(8), sy(y), sx(5), sy(35), 2, accents[i]);
      if (active) tft_.drawRoundRect(sx(8), sy(y), sx(304), sy(35), 7, accents[i]);
      text(20, y + 8, String(i + 1), 2, accents[i]);
      text(44, y + 6, names[i], 2, active ? accents[i] : ST77XX_WHITE);
      text(44, y + 25, details[i], 1, 0xBDF7);
    }
    text(12, 222, "MODE: next | RED: help | START: play", 1, 0x07FF);
  }

  void instructions(game::Mode mode) {
    lossAnimation_ = false;
    const bool memory = mode == game::Mode::Memory;
    const bool word = mode == game::Mode::Word;
    title("HOW TO PLAY", "START: go!      MODE: back to menu");
    text(12, 46, memory ? "MEMORY QUEST" : word ? "WORD RUSH" : "SPEED SPRINT", 2, 0x07FF);
    if (memory) {
      text(12, 79, "1. WATCH the color sequence.", 1);
      text(12, 99, "2. Wait for YOUR TURN.", 1);
      text(12, 119, "3. Repeat it in the same order.", 1);
      text(12, 144, "Clear all 10 levels to win!", 1, 0xFFE0);
      text(12, 164, "One mistake or timeout ends the game.", 1);
    } else {
      text(12, 79, word ? "Match the WORD, ignore the background." : "Press the matching COLOR button.", 1);
      text(12, 99, word ? "RED on blue? Press the RED button!" : "15 seconds. How fast can you go?", 1);
      text(12, 124, "Correct: +1 point", 2, 0x07E0);
      text(12, 149, "Every 5-hit streak: +2 bonus", 1, 0xFFE0);
      text(12, 169, "Wrong / multi-press resets the streak.", 1);
    }
    text(12, 194, "One button at a time. Release each hit.", 1);
  }
  void wordCard(uint8_t word, uint8_t background) {
    colour(background, false);
    const char* names[] = {"RED", "BLUE", "GREEN", "BROWN"};
    text((320 - strlen(names[word]) * 24) / 2, 104, names[word], 4);
  }
  void countdown(uint8_t n) {
    title("GET READY");
    text(142, 90, String(n), 6);
    text(40, 178, "Release all buttons");
  }
  void memory(uint32_t level, const char* phase,
              uint32_t step = 0) {
    const bool answer = strcmp(phase, "YOUR TURN") == 0;
    // Avoid full-screen SPI repaints between answers.
    if (!answer || step == 1) {
      title(answer ? "YOUR TURN - PRESS NOW" : "WATCH - WAIT");
      text(12, 54, String("Level ") + level);
      text(212, 54, "ONE CHANCE", 1);
    }
    if (answer) {
      if (step == 1) {
        text(94, 91, "YOUR TURN", 2, 0x07E0);
        text(40, 150, "ANSWER IN TIME OR YOU LOSE!", 1, 0xFFE0);
      }
      tft_.fillRect(sx(50), sy(120), sx(260), sy(28), ST77XX_BLACK);
      text(65, 122, String(step) + " / " + level, 3);
      const int completedWidth = static_cast<uint64_t>(step - 1) * 296 / level;
      tft_.fillRoundRect(sx(12), sy(162), sx(296), sy(8), 3, 0x2124);
      if (completedWidth) tft_.fillRect(sx(12), sy(162), sx(completedWidth), sy(8), 0x07E0);
      if (step == 1) for (uint8_t i = 0; i < 4; ++i)
        tft_.fillRoundRect(sx(36 + i * 64), sy(183), sx(54), sy(22), 5, game::buttonColours[i]);
    } else {
      text(66, 96, "EYES READY?", 3, 0xFFE0);
      text(34, 145, "Watch. Remember. Win!");
      text(34, 182, "Press after YOUR TURN", 1);
    }
  }
  void answerTimer(uint32_t tenths) {
    // Only repaint the timer so the sequence progress stays steady.
    tft_.fillRect(sx(12), sy(74), sx(296), sy(12), ST77XX_BLACK);
    text(12, 76, String("Time left: ") + (tenths / 10) + "." +
         (tenths % 10) + " sec", 1, tenths <= 10 ? 0xF800 : 0xFFE0);
  }
  void releaseForTurn() {
    title("SEQUENCE FINISHED");
    text(34, 92, "RELEASE ALL BUTTONS");
    text(34, 132, "Then repeat colors");
    text(34, 176, "Early presses ignored", 1);
  }
  // Playback contains only colors and plain black pauses, with no prompts.
  void watch(uint8_t color) { colour(color, false); }
  void gap() { clear(); }
  void speedCard(bool word, uint8_t color, uint8_t background, const char* cheer, uint16_t target, void (*service)()) {
    // Keep the target directly on the color background.
    uint16_t fill = game::buttonColours[word ? background : color];
#if TFT_SWAP_RED_BLUE
    fill = ((fill & 0x001F) << 11) | (fill & 0x07E0) | ((fill >> 11) & 0x001F);
#endif
    responsiveFill(0, 0, tft_.width(), tft_.height(), fill, service);
    text(12, 68, word ? "MATCH THE WORD" : "MATCH THE COLOR", 2);
    const char* names[] = {"RED", "BLUE", "GREEN", "BROWN"};
    text((320 - strlen(names[color]) * 24) / 2, 116, names[color], 4);
    text(12, 166, String("TARGET #") + target + " - release, then press", 1);
    responsiveFill(0, sy(184), tft_.width(), sy(56), ST77XX_BLACK, service);
    text(18, 190, word ? "Ignore the background color!" : "Press the matching color button", 1);
    text(18, 205, cheer, 1, 0xFFE0);
    text(18, 224, "START: menu", 1, 0xBDF7);
  }
  void speedStatus(uint16_t score, uint32_t seconds, uint16_t streak, bool hacked = false, void (*service)() = nullptr) {
    responsiveFill(0, 0, tft_.width(), sy(58), ST77XX_BLACK, service);
    text(12, 8, String("SCORE ") + score, 2);
    text(208, 8, String(seconds) + "s", 2, seconds <= 3 ? 0xF800 : 0xFFE0);
    text(12, 32, String("Combo ") + streak + " | " + (5 - streak % 5) + (hacked ? " hits to +4!" : " hits to +2!"), 1, 0xFFE0);
    for (uint8_t i = 0; i < 5; ++i)
      tft_.fillRect(sx(242 + i * 12), sy(32), sx(8), sy(7), i < streak % 5 ? 0x07FF : 0x2124);
    tft_.fillRect(sx(12), sy(48), sx(296), sy(5), 0x2124);
    tft_.fillRect(sx(12), sy(48), sx(296 * seconds / 15), sy(5), 0x07FF);
  }
  void feedback(bool success, uint32_t level,
                const char* reason) {
    const char* cheers[] = {"BRAIN ON FIRE!", "NAILED IT!", "MEMORY MAGIC!", "YOU ROCK!"};
    title(success ? (level == 3 ? "MEMORY SPARK!" : level == 5 ? "BRAIN POWER!" :
                     level == 8 ? "ALMOST A LEGEND!" : cheers[level % 4]) : "OOPS! TRY AGAIN");
    if (success) {
      for (int i = 0; i < 20; ++i) {
        const int x = 8 + (i * 47 + level % 13) % 300;
        const int y = 48 + (i * 31) % 145;
        tft_.fillRect(sx(x), sy(y), 4, 4, game::buttonColours[i % 4]);
      }
    }
    tft_.fillCircle(sx(160), sy(98), 30, success ? 0xFFE0 : 0xBDF7);
    tft_.fillCircle(sx(149), sy(91), 3, ST77XX_BLACK);
    tft_.fillCircle(sx(171), sy(91), 3, ST77XX_BLACK);
    tft_.drawLine(sx(148), sy(success ? 105 : 112), sx(160), sy(success ? 113 : 105), ST77XX_BLACK);
    tft_.drawLine(sx(160), sy(success ? 113 : 105), sx(172), sy(success ? 105 : 112), ST77XX_BLACK);
    text(42, 143, String("Level ") + level + " cleared!");
    text(40, 175, reason, 1);
    text(40, 195, success ? "Ready for one more?" : "You got this!");
  }
  void results(bool speed, uint32_t score, uint32_t best, const char* reason = "",
               uint8_t finalLevel = game::maxMemoryLevel,
               uint32_t attempts = 0, uint32_t hits = 0, bool newRecord = false, bool hacked = false) {
    const bool won = !speed && score == finalLevel;
    lossAnimation_ = !speed && !won;
    title(hacked ? "HACK RUN!" : newRecord ? "NEW RECORD!" : speed ? "TIME UP" : won ? "QUEST COMPLETE!" : "BRAIN WENT AFK!", "START: menu        MODE: menu");
    tft_.drawFastHLine(0, sy(39), tft_.width(), newRecord ? 0xFFE0 : 0x07FF);
    if (newRecord || won) {
      for (uint8_t i = 0; i < 26; ++i) {
        const int x = 8 + (i * 47) % 302;
        const int y = 43 + (i * 19) % 10;
        tft_.fillRect(sx(x), sy(y), sx(3), sy(3), i % 2 ? 0xF81F : 0xFFE0);
      }
    }
    if (speed) {
      const uint32_t hitPoints = hits * (hacked ? 2u : 1u);
      text(12, 58, String("Correct: ") + hits + (hacked ? " x 2" : " x 1"), 2, 0x07E0);
      text(224, 58, String(hitPoints) + " pts", 2, 0x07E0);
      text(12, 84, "Streak bonus", 2, 0xFFE0);
      text(224, 84, String("+") + (score - hitPoints), 2, 0xFFE0);
      tft_.drawFastHLine(sx(12), sy(106), sx(296), 0xBDF7);
      tft_.fillRoundRect(sx(8), sy(112), sx(304), sy(30), 5, 0x1082);
      text(16, 119, "TOTAL POINTS", 2, 0x07FF);
      text(224, 119, String(score), 2, 0x07FF);
      text(12, 151, String("Answers: ") + hits + " correct / " + attempts + " attempts", 1);
      text(12, 167, String("Accuracy: ") + (attempts ? hits * 100 / attempts : 0) + "%", 1, 0xBDF7);
      recordBanner(best, score, true, hacked, newRecord, finalLevel);
      return;
    }
    text(12, 58, "LEVELS CLEARED", 1, 0xBDF7);
    text(12, 82, String(score) + "/" + finalLevel, 4);
    text(12, 126, String("Attempts: ") + attempts, 2);
    text(12, 150, String("Correct:  ") + hits, 2, 0x07E0);
    text(12, 115, reason, 1, 0xBDF7);
    recordBanner(best, score, false, hacked, newRecord, finalLevel);
    if (!won) {
      lastLossFrame_ = millis();
      drawSillyFace(false);
    }
  }

  void animateLoss(uint32_t now) {
    if (!lossAnimation_ || !game::elapsed(now, lastLossFrame_, 300)) return;
    lastLossFrame_ = now;
    lossFrame_ = !lossFrame_;
    drawSillyFace(lossFrame_);
  }
  void diagnostic(uint8_t held, uint8_t checked, bool speed, uint8_t modes) {
    title("HARDWARE TEST", "Press/release all; flip MODE both ways");
    const char* names[] = {"RED", "BLUE", "GREEN", "BROWN", "START"};
    for (uint8_t i = 0; i < 5; ++i) {
      const bool down = held & (1 << i);
      const String status = String(names[i]) + ": " +
          (down ? "DOWN" : "UP") + ((checked & (1 << i)) ? "  OK" : "  --");
      text(12, 46 + i * 24, status, 2, down ? 0xFFE0 : ST77XX_WHITE);
    }
    text(12, 170, String(speed ? "MODE: SPEED" : "MODE: MEMORY") +
         (modes == 3 ? "  OK" : "  flip switch"), 1);
    for (uint8_t i = 0; i < 6; ++i) {
      uint16_t color = i < 4 ? game::buttonColours[i] : i == 4 ? 0xFFFF : 0;
#if TFT_SWAP_RED_BLUE
      color = ((color & 0x001F) << 11) | (color & 0x07E0) | ((color >> 11) & 0x001F);
#endif
      tft_.fillRect(sx(8 + i * 50), sy(187), sx(50), sy(22), color);
    }
    text(12, 211, "RED  BLUE GREEN BROWN WHITE BLACK", 1);
    tft_.drawRect(0, 0, tft_.width(), tft_.height(), ST77XX_WHITE);
  }
 private:
  bool lossAnimation_ = false;
  bool lossFrame_ = false;
  uint32_t lastLossFrame_ = 0;
  void responsiveFill(int16_t x, int16_t y, int16_t width, int16_t height,
                      uint16_t color, void (*service)()) {
    // At 4 MHz a full-screen transfer is long enough to hide an entire tap.
    // Eight-row strips keep release/debounce sampling active during painting.
    for (int16_t row = 0; row < height; row += 8) {
      const int16_t remaining = height - row;
      tft_.fillRect(x, y + row, width, remaining < 8 ? remaining : 8, color);
      if (service) service();
    }
  }
  void recordBanner(uint32_t best, uint32_t score, bool speed, bool hacked,
                    bool newRecord, uint8_t finalLevel) {
    tft_.fillRoundRect(sx(8), sy(178), sx(304), sy(35), 6, 0x2124);
    text(16, 182, hacked ? "RECORD UNCHANGED" : newRecord ? "NEW RECORD!" : "RECORD TO BEAT", 1, 0xFFE0);
    text(232, 182, String(best), 3, 0xFFE0);
    const String challenge = hacked ? String("Cheat run - not saved") :
        !speed && best == finalLevel ? String("Perfect record! Match all 10 levels.") :
        String("Next goal: ") + (best + 1) + (speed ? " points" : " levels");
    text(16, 199, challenge, 1, 0xBDF7);
    (void)score;
  }
  void drawSillyFace(bool frame) {
    // Repaint only the mascot; results stay readable and START stays responsive.
    tft_.fillRect(sx(210), sy(60), sx(105), sy(62), ST77XX_BLACK);
    const int x = sx(260), y = sy(86);
    tft_.fillCircle(x, y, 24, 0xFFE0);
    for (int eye = -12; eye <= 12; eye += 24) {
      tft_.drawLine(x + eye - 4, y - 12, x + eye + 4, y - 4, ST77XX_BLACK);
      tft_.drawLine(x + eye - 4, y - 4, x + eye + 4, y - 12, ST77XX_BLACK);
    }
    tft_.fillRoundRect(x - 15, y + 7, 30, 13, 5, ST77XX_BLACK);
    tft_.fillRoundRect(x + (frame ? 0 : -8), y + 13, 12, frame ? 14 : 10, 4, 0xFBCF);
    text(294, 62, frame ? "?" : "!", 2, ST77XX_WHITE);
  }
  int16_t sx(int16_t value) const { return value * tft_.width() / 320; }
  int16_t sy(int16_t value) const { return value * tft_.height() / 240; }
  void overlay(const char* heading, const String& status, const String& footer) {
    tft_.fillRect(0, 0, tft_.width(), sy(64), ST77XX_BLACK);
    tft_.fillRect(0, tft_.height() - sy(24), tft_.width(), sy(24), ST77XX_BLACK);
    text(12, 12, heading);
    text(12, 44, status, 1);
    text(12, 222, footer, 1);
  }
  Adafruit_ST7789 tft_{5, 16, 4};
};
}  // namespace hardware
static hardware::GameDisplay gameDisplay;
#else
#define gameDisplay Serial
#endif

#ifdef ARDUINO_ARCH_ESP32
#define SCREEN(view) gameDisplay.view
#else
#define SCREEN(view) do {} while (0)
#endif
