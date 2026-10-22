#pragma once
#include <stdint.h>
#include <sstream>
#include <string>
#include <vector>
#define LOW 0
#define HIGH 1
#define INPUT_PULLUP 2
#define A0 14
class __FlashStringHelper;
#define F(value) reinterpret_cast<const __FlashStringHelper*>(value)
struct PinEvent { uint32_t time; uint8_t pin; int value; };
extern uint32_t fakeTime;
extern int fakePins[40];
extern std::vector<PinEvent> events;
extern unsigned randomIndex;
extern unsigned eventCursor;
inline uint32_t millis() { return fakeTime; }
inline uint32_t micros() { return fakeTime * 1000; }
inline void delay(uint32_t duration) { fakeTime += duration; }
inline void pinMode(uint8_t, int) {}
inline int digitalRead(uint8_t pin) {
  while (eventCursor < events.size() && events[eventCursor].time <= fakeTime) {
    const auto& event = events[eventCursor++];
    fakePins[event.pin] = event.value;
  }
  return fakePins[pin];
}
inline int analogRead(int) { return 42; }
inline void randomSeed(unsigned long) { randomIndex = 0; }
inline long random(long minimum, long maximum) { return minimum + randomIndex++ % (maximum - minimum); }
inline long random(long limit) { return randomIndex++ % limit; }
struct FakeSerial {
  std::string output;
  void begin(unsigned long) {}
  void flush() {}
  void print(const __FlashStringHelper* value) {
    output += reinterpret_cast<const char*>(value);
  }
  void print(uint8_t value) { print(static_cast<unsigned>(value)); }
  template <typename T> void print(T value) {
    std::ostringstream stream;
    stream << value;
    output += stream.str();
  }
  void println() { output += '\n'; }
  template <typename T> void println(T value) { print(value); println(); }
};
extern FakeSerial Serial;
