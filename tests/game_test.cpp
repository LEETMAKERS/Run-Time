#include <assert.h>
#include <stdio.h>
#include "../main.cpp"

uint32_t fakeTime = 0;
int fakePins[20];
std::vector<PinEvent> events;
unsigned randomIndex = 0;
unsigned eventCursor = 0;
FakeSerial Serial;

void reset() {
  fakeTime = randomIndex = eventCursor = 0;
  for (int& pin : fakePins) pin = HIGH;
  events.clear();
  Serial.output.clear();
  inputs = hardware::Inputs();
  inputs.begin();
  bestMemory = bestSpeed = 0;
}
void press(uint32_t at, uint8_t pin) {
  events.push_back({at, pin, LOW});
  events.push_back({at + 50, pin, HIGH});
}
int main() {
  reset();
  fakePins[2] = LOW;
  inputs.begin();
  assert(waitForAnswer(100) == timeout);
  assert(fakeTime == 100); // A held button cannot stall a deadline.

  reset();
  press(10, 2);
  assert(waitForAnswer(100) == 0);
  assert(fakeTime == 35); // Count a debounced press, not its release.
  assert(waitForAnswer(100) == timeout); // No duplicate answer.

  reset();
  press(10, hardware::startPin);
  assert(waitForAnswer(100) == cancelled);

  reset();
  speedGame();
  assert(fakeTime == 2100 + game::speedDurationMs);
  assert(Serial.output.find("Score: 0") != std::string::npos);

  reset();
  memoryGame(); // No answers: three timeouts, two retries, clean exit.
  assert(Serial.output.find("Lives left: 0") != std::string::npos);
  assert(Serial.output.find("Levels completed: 0") != std::string::npos);
  assert(bestMemory == 0);

  reset();
  press(2200, hardware::startPin); // Cancel during sequence preparation.
  memoryGame();
  assert(fakeTime == 2225);

  reset();
  uint32_t at = 2100;
  for (uint8_t level = 1; level <= game::maxLevel; ++level) {
    const uint8_t length = game::sequenceLength(level);
    at += 1300 + length * (game::showTime(level) + game::gapTime(level));
    for (uint8_t index = 0; index < length; ++index) {
      press(at + 50, hardware::colourPins[index % 4]);
      at += 75; // waitForAnswer returns after debounce.
      if (index + 1 < length) at += 25; // Space the next gesture after release.
    }
    at += 1800;
  }
  memoryGame();
  assert(bestMemory == game::maxLevel);
  assert(Serial.output.find("MEMORY MASTER!") != std::string::npos);
  puts("PASS: answer deadlines, cancellation, sprint, memory retries, all 40 levels");
}
