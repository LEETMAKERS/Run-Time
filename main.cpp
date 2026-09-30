#include "include/hardware.h"

namespace {
hardware::Inputs inputs;
uint16_t bestSpeed = 0;
uint8_t bestMemory = 0;
constexpr int8_t timeout = -1;
constexpr int8_t cancelled = -3;

void clearScreen() {
  Serial.print(F("\033[2J\033[H"));
}

bool pauseFor(uint32_t duration) {
  const uint32_t began = millis();
  while (!game::elapsed(millis(), began, duration)) {
    inputs.poll();
    if (inputs.start.pressed) return false;
    delay(1);
  }
  return true;
}

bool countdown() {
  Serial.println(F("Get ready! START returns to the menu."));
  for (uint8_t n = 3; n > 0; --n) {
    Serial.println(n);
    if (!pauseFor(700)) return false;
  }
  return true;
}

int8_t waitForAnswer(uint32_t limit) {
  const uint32_t began = millis();
  while (!game::elapsed(millis(), began, limit)) {
    inputs.poll();
    if (inputs.start.pressed) return cancelled;
    if (inputs.answer != timeout) return inputs.answer;
    delay(1);
  }
  return timeout;
}

void showColour(uint8_t colour) {
  Serial.print(F(">>> "));
  Serial.print(hardware::colourName(colour));
  Serial.println(F(" <<<"));
}

void speedGame() {
  clearScreen();
  Serial.println(F("Run Time - 15 SECOND SPRINT"));
  Serial.println(F("Match the colour! Correct: +1. Every 5-hit streak: +2 bonus."));
  Serial.println(F("Wrong or multiple buttons reset your streak. Release between hits."));
  if (!countdown()) return;

  uint16_t score = 0, hits = 0, attempts = 0, streak = 0;
  int8_t previous = -1;
  inputs.gate.reset();
  const uint32_t began = millis();
  while (!game::elapsed(millis(), began, game::speedDurationMs)) {
    uint8_t colour;
    do { colour = random(game::colourCount); } while (colour == previous);
    previous = colour;
    showColour(colour);
    const uint32_t used = static_cast<uint32_t>(millis() - began);
    if (used >= game::speedDurationMs) break;
    const int8_t answer = waitForAnswer(game::speedDurationMs - used);
    if (answer == cancelled) return;
    if (answer == timeout) break;
    ++attempts;
    if (answer == colour) {
      ++hits;
      ++score;
      ++streak;
      Serial.println(F("Nice! +1"));
      if (streak % 5 == 0) {
        score += 2;
        Serial.println(F("ON FIRE! Streak bonus +2!"));
      }
    } else {
      streak = 0;
      Serial.println(F("Oops! Streak reset. Keep going!"));
    }
  }
  Serial.println(F("TIME!"));
  Serial.print(F("Score: ")); Serial.println(score);
  Serial.print(F("Correct / attempts: ")); Serial.print(hits);
  Serial.print(F(" / ")); Serial.println(attempts);
  if (score > bestSpeed) {
    bestSpeed = score;
    Serial.println(F("NEW SESSION RECORD!"));
  }
  Serial.print(F("Session best: ")); Serial.println(bestSpeed);
}

bool showSequence(const uint8_t* sequence, uint8_t level, uint8_t lives) {
  clearScreen();
  Serial.print(F("MEMORY QUEST - Level ")); Serial.println(level);
  Serial.print(F("Colours: ")); Serial.print(game::sequenceLength(level));
  Serial.print(F(" | Lives: ")); Serial.println(lives);
  Serial.print(F("Time per answer: ")); Serial.print(game::answerTime(level));
  Serial.println(F(" ms"));
  Serial.println(F("WATCH! Hands off the buttons."));
  if (!pauseFor(1300)) return false;
  for (uint8_t i = 0; i < game::sequenceLength(level); ++i) {
    clearScreen();
    Serial.print(F("WATCH - ")); Serial.print(i + 1);
    Serial.print('/'); Serial.println(game::sequenceLength(level));
    showColour(sequence[i]);
    Serial.flush();  // Start the visible interval after transmission finishes.
    if (!pauseFor(game::showTime(level))) return false;
    clearScreen();
    Serial.flush();
    if (!pauseFor(game::gapTime(level))) return false;
  }
  return true;
}

void memoryGame() {
  clearScreen();
  Serial.println(F("MEMORY QUEST"));
  Serial.println(F("Watch, then repeat the colours in order. Start with 3 colours."));
  Serial.println(F("Each level adds one colour and gets faster. You have 3 lives."));
  Serial.println(F("Mistakes replay the same level. Every 5 levels: +1 life (max 3)."));
  Serial.println(F("Press one button at a time; release before the next answer."));
  if (!countdown()) return;

  uint8_t sequence[game::maxLevel + 2];
  for (uint8_t& colour : sequence) colour = random(game::colourCount);
  uint8_t lives = game::memoryLives;
  uint8_t completed = 0;
  for (uint8_t level = 1; level <= game::maxLevel;) {
    if (!showSequence(sequence, level, lives)) return;
    Serial.println(F("YOUR TURN! Release all colours, then repeat the sequence."));
    inputs.gate.reset();
    bool success = true;
    for (uint8_t i = 0; i < game::sequenceLength(level); ++i) {
      Serial.print(F("Answer ")); Serial.print(i + 1);
      Serial.print('/'); Serial.println(game::sequenceLength(level));
      const int8_t answer = waitForAnswer(game::answerTime(level));
      if (answer == cancelled) return;
      if (answer != sequence[i]) {
        success = false;
        if (answer == timeout) Serial.println(F("Time ran out!"));
        else if (answer == -2) Serial.println(F("One colour at a time!"));
        else Serial.println(F("Oops, that colour slipped away!"));
        Serial.print(F("Expected: ")); Serial.println(hardware::colourName(sequence[i]));
        break;
      }
      Serial.println(F("Yes!"));
    }
    if (!success) {
      --lives;
      Serial.print(F("Lives left: ")); Serial.println(lives);
      if (!lives) break;
      Serial.println(F("You can do it! Watch the same sequence again."));
    } else {
      completed = level;
      if (completed > bestMemory) bestMemory = completed;
      Serial.println(F("LEVEL CLEARED! Brain power +1!"));
      if (level % 5 == 0) {
        if (lives < game::memoryLives) ++lives;
        Serial.println(F("MILESTONE! Life recharged. Next challenge unlocked!"));
      }
      ++level;
    }
    if (!pauseFor(1800)) return;
  }
  Serial.println(completed == game::maxLevel ? F("MEMORY MASTER! All 40 levels conquered!")
                                            : F("QUEST OVER! Ready for a comeback?"));
  Serial.print(F("Levels completed: ")); Serial.println(completed);
  Serial.print(F("Session best: ")); Serial.println(bestMemory);
}

void showMenu() {
  Serial.println(F("\n========== Run Time =========="));
  Serial.println(inputs.mode.down() ? F("Selected: SPEED SPRINT") : F("Selected: MEMORY QUEST"));
  Serial.print(F("Best sprint: ")); Serial.print(bestSpeed);
  Serial.print(F(" | Best memory level: ")); Serial.println(bestMemory);
  Serial.println(F("Flip MODE to choose. Press and release START to play."));
  Serial.println(F("During a game, press START to quit. Records reset on power off."));
}
}  // namespace

void setup() {
  inputs.begin();
  Serial.begin(hardware::baudRate);
  showMenu();
}

void loop() {
  static bool startPending = false;
  inputs.poll();
  if (inputs.mode.pressed || inputs.mode.released) showMenu();
  if (inputs.start.pressed) startPending = true;
  if (startPending && inputs.start.released) {
    startPending = false;
          randomSeed(analogRead(A0) ^ micros());
    if (inputs.mode.down()) speedGame();
    else memoryGame();
    showMenu();
  }
  delay(1);
}
