#pragma once

#include <stdint.h>

namespace game {
constexpr uint8_t colourCount = 4;
constexpr uint8_t maxLevel = 40;
constexpr uint8_t memoryLives = 3;
constexpr uint32_t speedDurationMs = 15000;
constexpr uint32_t debounceMs = 25;

inline uint8_t sequenceLength(uint8_t level) { return level + 2; }
inline uint16_t boundedTime(uint8_t level, uint16_t initial,
                            uint16_t decrease, uint16_t minimum) {
  const int32_t value = initial - static_cast<int32_t>(level - 1) * decrease;
  return value < minimum ? minimum : static_cast<uint16_t>(value);
}
inline uint16_t showTime(uint8_t level) {
  return boundedTime(level, 1000, 20, 220);
}
inline uint16_t answerTime(uint8_t level) {
  return boundedTime(level, 4000, 75, 1075);
}
inline uint16_t gapTime(uint8_t level) {
  return boundedTime(level, 300, 5, 105);
}

// Unsigned subtraction keeps timers correct across millis() rollover.
inline bool elapsed(uint32_t now, uint32_t since, uint32_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}

class Button {
 public:
  void begin(bool down, uint32_t now) {
    raw_ = stable_ = down;
    changedAt_ = now;
    pressed = released = false;
  }
  void update(bool down, uint32_t now) {
    pressed = released = false;
    if (down != raw_) {
      raw_ = down;
      changedAt_ = now;
    }
    if (raw_ != stable_ && elapsed(now, changedAt_, debounceMs)) {
      stable_ = raw_;
      pressed = stable_;
      released = !stable_;
    }
  }
  bool down() const { return stable_; }
  bool pressed = false;
  bool released = false;

 private:
  bool raw_ = false;
  bool stable_ = false;
  uint32_t changedAt_ = 0;
};

// A gesture can yield only one answer; release every colour before another.
class ColourGate {
 public:
  void reset() { armed_ = false; }
  int8_t update(uint8_t mask) {
    if (!mask) {
      armed_ = true;
      return -1;
    }
    if (!armed_) return -1;
    armed_ = false;
    if (mask & (mask - 1)) return -2;  // Multiple buttons, never a valid answer.
    for (uint8_t i = 0; i < colourCount; ++i) {
      if (mask & (1 << i)) return i;
    }
    return -2;
  }
 private:
  bool armed_ = false;
};
}  // namespace game
