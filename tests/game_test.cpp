#include <assert.h>
#include <stdio.h>
#include "../main.cpp"

uint32_t fakeTime = 0;
int fakePins[40];
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
  bestMemory = bestSpeed = bestWord = 0;
  cheatEnabled = false;
  cheatCode.reset();
  selectedMode = game::Mode::Memory;
}
void press(uint32_t at, uint8_t pin) {
  events.push_back({at, pin, LOW});
  events.push_back({at + 50, pin, HIGH});
}
int main() {
  reset();
  fakePins[hardware::colourPins[0]] = LOW;
  inputs.begin();
  assert(waitForAnswer(100) == timeout);
  assert(fakeTime == 100); // A held button cannot stall a deadline.

  reset();
  assert(waitForAnswer(4000, true) == timeout);
  assert(fakeTime == 4000); // Timer drawing must not extend the deadline.

  reset();
  press(10, hardware::colourPins[0]);
  assert(waitForAnswer(100) == 0);
  assert(fakeTime == 35); // Count a debounced press, not its release.
  assert(waitForAnswer(100) == timeout); // No duplicate answer.

  reset();
  press(10, hardware::startPin);
  assert(waitForAnswer(100) == cancelled);

  reset();
  press(750, hardware::colourPins[0]); // Early press during playback.
  assert(showSequence(1, 1));
  assert(prepareMemoryTurn());
  assert(waitForAnswer(100) == timeout); // No playback input was queued.

  reset();
  events.push_back({750, hardware::colourPins[0], LOW});
  events.push_back({2800, hardware::colourPins[0], HIGH});
  press(2900, hardware::colourPins[1]);
  assert(showSequence(1, 1));
  assert(prepareMemoryTurn());
  assert(fakeTime == 2825); // Wait for debounced release before enabling answers.
  assert(waitForAnswer(200) == 1); // Only the fresh press counts.

  reset();
  fakePins[hardware::colourPins[0]] = LOW;
  inputs.begin();
  press(10, hardware::startPin);
  assert(!prepareMemoryTurn()); // START still cancels while waiting for release.

  reset();
  press(10, hardware::startPin);
  assert(howToPlay(game::Mode::Word));
  assert(fakeTime == 85); // Continue only after START is released.
  assert(Serial.output.find("RED on blue means press RED") != std::string::npos);

  reset();
  events.push_back({10, hardware::modePin, LOW});
  assert(!howToPlay(game::Mode::Memory));
  assert(fakeTime == 35);

  reset();
  speedAnswer = timeout;
  press(10, hardware::colourPins[0]);
  // Simulate input service calls while a slow display paints eight-row strips.
  for (unsigned i = 0; i < 30; ++i) { pollSpeedInput(); delay(5); }
  assert(speedAnswer == 0); // A complete tap during drawing is retained.
  pollSpeedInput();
  assert(inputs.answer == timeout);
  assert(speedAnswer == 0); // Later polls cannot erase the latched edge.
  speedAnswer = timeout;
  press(fakeTime + 10, hardware::colourPins[0]);
  for (unsigned i = 0; i < 30; ++i) { pollSpeedInput(); delay(5); }
  assert(speedAnswer == 0); // Release during drawing rearmed the next tap.
  press(fakeTime + 10, hardware::startPin);
  for (unsigned i = 0; i < 10; ++i) { pollSpeedInput(); delay(5); }
  assert(speedAnswer == cancelled); // START takes priority over a latched answer.

  reset();
  speedGame();
  assert(fakeTime == 2100 + game::speedDurationMs);
  assert(Serial.output.find("Score: 0") != std::string::npos);

  reset();
  speedGame(true);
  assert(fakeTime == 2100 + game::speedDurationMs);
  assert(Serial.output.find("WORD RUSH") != std::string::npos);

  reset();
  for (uint8_t i = 0; i < 5; ++i)
    press(2200 + i * 100, hardware::colourPins[i % 2 ? 2 : 0]);
  speedGame(true);
  assert(bestWord == 7); // Five correct words plus the streak bonus.
  assert(bestSpeed == 0); // Separate record from Speed Sprint.
  assert(Serial.output.find("Correct / attempts: 5 / 5") != std::string::npos);

  reset();
  press(2200, hardware::startPin);
  speedGame(true);
  assert(fakeTime == 2225);
  assert(bestWord == 0);

  reset();
  cheatEnabled = true;
  for (uint8_t i = 0; i < 5; ++i)
    press(2200 + i * 100, hardware::colourPins[i % 2 ? 2 : 0]);
  speedGame(true);
  assert(bestWord == 0);
  assert(Serial.output.find("Score: 14") != std::string::npos);

  reset();
  cheatEnabled = true;
  memoryGame();
  assert(fakeTime == 2100u + game::memoryWatchPauseMs + game::showTime(1)
                     + game::gapTime(1) + 2 * game::answerTime(1));

  reset();
  memoryGame(); // A single timeout ends the game.
  assert(fakeTime == 2100u + game::memoryWatchPauseMs + game::showTime(1) + game::gapTime(1) + game::answerTime(1));
  assert(Serial.output.find("MEMORY QUEST - Level 1") == Serial.output.rfind("MEMORY QUEST - Level 1"));
  assert(Serial.output.find("Levels completed: 0") != std::string::npos);
  assert(bestMemory == 0);
  assert(Serial.output.find("Attempts: 0") != std::string::npos);
  assert(Serial.output.find("Correct: 0") != std::string::npos);

  reset();
  press(3750, hardware::colourPins[0]); // Seed 1 starts blue; red is wrong.
  memoryGame();
  assert(Serial.output.find("Attempts: 1") != std::string::npos);
  assert(Serial.output.find("Correct: 0") != std::string::npos);
  assert(fakeTime == 3775); // Ends on the first debounced wrong press.
  assert(bestMemory == 0);
  assert(Serial.output.find("MEMORY QUEST - Level 2") == std::string::npos);

  reset();
  press(2200, hardware::startPin); // Cancel during sequence preparation.
  memoryGame();
  assert(fakeTime == 2225);

  reset();
  uint32_t at = 2100;
  for (uint8_t level = 1; level <= game::maxMemoryLevel; ++level) {
    const uint8_t length = game::sequenceLength(level);
    at += game::memoryWatchPauseMs + length * (game::showTime(level) + game::gapTime(level));
    game::Sequence sequence(level);
    for (uint8_t index = 0; index < length; ++index) {
      press(at + 50, hardware::colourPins[sequence.next()]);
      at += 75; // waitForAnswer returns after debounce.
      if (index + 1 < length) at += 25; // Space the next gesture after release.
    }
    if (level < game::maxMemoryLevel) at += game::memoryCelebrationMs;
  }
  press(at + 100, hardware::startPin);
  memoryGame();
  assert(bestMemory == game::maxMemoryLevel);
  assert(fakeTime == at);
  assert(randomIndex == game::maxMemoryLevel);
  assert(Serial.output.find("QUEST COMPLETE!") != std::string::npos);
  assert(Serial.output.find("Levels completed: 10") != std::string::npos);
  assert(Serial.output.find("Level 11") == std::string::npos);
  reset();
  for (unsigned i = 0; i < 3; ++i) {
    const uint32_t now = fakeTime;
    events.push_back({now, hardware::modePin, i % 2 ? HIGH : LOW});
    loop();
    delay(25);
    loop();
    assert(selectedMode == static_cast<game::Mode>((i + 1) % 3));
  }

  reset();
  memoryGame(true);
  assert(bestWord == 0);
  assert(fakeTime == 2100u + game::memoryWatchPauseMs + game::wordShowTime(1)
                     + game::gapTime(1) + game::wordAnswerTime(1));

  reset();
  at = 2100;
  for (uint8_t level = 1; level <= game::maxWordLevel; ++level) {
    at += game::memoryWatchPauseMs + level * (game::wordShowTime(level) + game::gapTime(level));
    game::Sequence sequence(level);
    for (uint8_t index = 0; index < level; ++index) {
      press(at + 50, hardware::colourPins[sequence.next()]);
      at += 75;
      if (index + 1 < level) at += 25;
    }
    if (level < game::maxWordLevel) at += game::memoryCelebrationMs;
  }
  memoryGame(true);
  assert(Serial.output.find("Attempts: 55") != std::string::npos);
  assert(Serial.output.find("Correct: 55") != std::string::npos);
  assert(bestWord == game::maxWordLevel);
  assert(bestMemory == 0);
  assert(fakeTime == at);
  assert(randomIndex == game::maxWordLevel);
  assert(Serial.output.find("All 10 levels cleared!") != std::string::npos);
  assert(Serial.output.find("Level 11") == std::string::npos);
  puts("PASS: answer deadlines, cancellation, sprint, memory sudden death, 10-level victory");
}
