#pragma once

#include <stdint.h>

namespace game {
constexpr uint8_t colourCount = 4;
constexpr uint8_t maxMemoryLevel = 10;
constexpr uint8_t maxWordLevel = 10;
enum class Mode : uint8_t { Memory, Speed, Word };
inline Mode nextMode(Mode mode) {
  return static_cast<Mode>((static_cast<uint8_t>(mode) + 1) % 3);
}
constexpr uint16_t memoryWatchPauseMs = 700;
constexpr uint16_t memoryCelebrationMs = 350;
constexpr uint32_t speedDurationMs = 15000;
constexpr uint32_t debounceMs = 25;

inline uint32_t sequenceLength(uint32_t level) { return level; }

// A seeded stream replays a round, allowing at most two identical colors in a row.
class Sequence {
 public:
  explicit Sequence(uint32_t seed) : state_(seed ? seed : 1) {}
  uint8_t next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    uint8_t colour = state_ % colourCount;
    if (colour == previous_ && repeats_ == 2) {
      // Pick one of the other three colors without an unbounded retry loop.
      colour = (previous_ + 1 + (state_ / colourCount) % (colourCount - 1)) % colourCount;
    }
    repeats_ = colour == previous_ ? repeats_ + 1 : 1;
    previous_ = colour;
    return colour;
  }
 private:
  uint32_t state_;
  uint8_t previous_ = colourCount;
  uint8_t repeats_ = 0;
};
inline uint16_t boundedTime(uint32_t level, uint16_t initial,
                            uint16_t decrease, uint16_t minimum) {
  const int64_t value = initial - static_cast<int64_t>(level - 1) * decrease;
  return value < minimum ? minimum : static_cast<uint16_t>(value);
}
inline uint16_t wordShowTime(uint32_t level) {
  return boundedTime(level, 600, 40, 240);
}
inline uint16_t wordAnswerTime(uint32_t level) {
  return boundedTime(level, 2500, 175, 1000);
}
inline uint16_t showTime(uint32_t level) {
  return boundedTime(level, 650, 30, 220);
}
inline uint16_t answerTime(uint32_t level) {
  return boundedTime(level, 4000, 75, 1075);
}
inline uint16_t gapTime(uint32_t level) {
  return boundedTime(level, 180, 8, 105);
}

// Unsigned subtraction keeps timers correct across millis() rollover.
inline bool elapsed(uint32_t now, uint32_t since, uint32_t duration) {
  return static_cast<uint32_t>(now - since) >= duration;
}

// Menu-only secret: Blue, Green, Brown, Green, Blue. Release between steps.
class CheatCode {
 public:
  void reset() { step_ = 0; }
  bool enter(int8_t colour, uint32_t now) {
    if (colour == -1) return false; // No new gesture.
    if (step_ && elapsed(now, lastAt_, 3000)) reset();
    lastAt_ = now;
    const int8_t code[] = {1, 2, 3, 2, 1};
    if (colour != code[step_]) {
      step_ = colour == code[0] ? 1 : 0;
      return false;
    }
    if (++step_ == 5) { reset(); return true; }
    return false;
  }
 private:
  uint8_t step_ = 0;
  uint32_t lastAt_ = 0;
};

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
