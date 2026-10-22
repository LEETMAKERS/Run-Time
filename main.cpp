#include "include/hardware.h"
#include "include/display.h"
#ifdef ARDUINO_ARCH_ESP32
#include <esp_system.h>
#include <Preferences.h>
#endif

namespace {
hardware::Inputs inputs;
game::CheatCode cheatCode;
bool cheatEnabled = false;
uint16_t bestSpeed = 0;
uint32_t bestMemory = 0;
uint32_t bestWord = 0;
game::Mode selectedMode = game::Mode::Memory;
constexpr int8_t timeout = -1;
constexpr int8_t cancelled = -3;

// NVS keeps records across resets and power loss. Write only improved records.
#ifdef ARDUINO_ARCH_ESP32
Preferences records;
bool recordsReady = false;
#endif

void loadRecords() {
#ifdef ARDUINO_ARCH_ESP32
  recordsReady = records.begin("runtime-best", false);
  if (!recordsReady) {
    gameDisplay.println(F("Record storage unavailable; best scores are temporary."));
    return;
  }
  bestSpeed = records.getUShort("speed", 0);
  bestMemory = records.getUInt("memory", 0);
  bestWord = records.getUInt("word", 0);
#endif
}

void saveRecord(game::Mode mode) {
#ifdef ARDUINO_ARCH_ESP32
  if (!recordsReady || cheatEnabled) return;
  size_t written = 0;
  if (mode == game::Mode::Speed) written = records.putUShort("speed", bestSpeed);
  else if (mode == game::Mode::Memory) written = records.putUInt("memory", bestMemory);
  else written = records.putUInt("word", bestWord);
  if (!written) gameDisplay.println(F("Could not save record to flash."));
#else
  (void)mode;
#endif
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

bool howToPlay(game::Mode mode) {
  SCREEN(instructions(mode));
  gameDisplay.println(F("HOW TO PLAY - START: go, MODE: menu"));
  if (mode == game::Mode::Memory) {
    gameDisplay.println(F("Watch the sequence. Repeat after YOUR TURN. Clear 10 levels!"));
  } else {
    gameDisplay.println(mode == game::Mode::Word
        ? F("Match the WORD. Ignore the background. RED on blue means press RED.")
        : F("Match the displayed COLOR. Get as many as you can in 15 seconds!"));
    gameDisplay.println(F("Correct = +1. Every 5 consecutive hits = +2 bonus. Wrong resets streak."));
  }
  gameDisplay.println(F("Press one color at a time. Release between answers."));
  bool ready = false;
  for (;;) {
    inputs.poll();
    if (inputs.mode.pressed || inputs.mode.released) return false;
    if (inputs.start.pressed) ready = true;
    if (ready && inputs.start.released) return true;
    delay(1);
  }
}

bool countdown() {
  gameDisplay.println(F("Get ready! START returns to the menu."));
  for (uint8_t n = 3; n > 0; --n) {
    SCREEN(countdown(n));
    gameDisplay.println(n);
    if (!pauseFor(700)) return false;
  }
  return true;
}

int8_t waitForAnswer(uint32_t limit, bool liveTimer = false) {
  const uint32_t began = millis();
  uint32_t lastTick = UINT32_MAX;
  while (!game::elapsed(millis(), began, limit)) {
    if (liveTimer) {
      const uint32_t tick = (limit - static_cast<uint32_t>(millis() - began) + 99) / 100;
      if (tick != lastTick) {
        SCREEN(answerTimer(tick));
        lastTick = tick;
      }
    }
    inputs.poll();
    if (inputs.start.pressed) return cancelled;
    if (inputs.answer != timeout) return inputs.answer;
    delay(1);
  }
  if (liveTimer) { SCREEN(answerTimer(0)); }
  return timeout;
}

void showColour(uint8_t colour) {
  gameDisplay.print(F(">>> "));
  gameDisplay.print(hardware::colourName(colour));
  gameDisplay.println(F(" <<<"));
}

void waitForResults() {
#ifdef ARDUINO_ARCH_ESP32
  gameDisplay.println(F("START: menu | MODE: menu"));
  bool startReady = false;
  for (;;) {
    inputs.poll();
    gameDisplay.animateLoss(millis());
    if (inputs.mode.pressed || inputs.mode.released) return;
    if (inputs.start.pressed) startReady = true;
    if (startReady && inputs.start.released) return;
    delay(1);
  }
#endif
}

// Latch an answer while SPI is painting, so a redraw cannot swallow its edge.
int8_t speedAnswer = timeout;
void pollSpeedInput() {
  inputs.poll();
  if (inputs.start.pressed) speedAnswer = cancelled;
  else if (speedAnswer == timeout && inputs.answer != timeout) speedAnswer = inputs.answer;
}

void speedGame(bool word = false) {
  uint32_t& best = bestWord;
  gameDisplay.println(word ? F("WORD RUSH: match the WORD, ignore the background!") : F("SPEED SPRINT: match the colour!"));
  gameDisplay.println(F("Run Time - 15 SECOND SPRINT"));
  gameDisplay.println(F("Match the colour! Correct: +1. Every 5-hit streak: +2 bonus."));
  gameDisplay.println(F("Wrong or multiple buttons reset your streak. Release between hits."));
  if (!countdown()) return;

  uint16_t score = 0, hits = 0, attempts = 0, streak = 0;
  int8_t previous = -1;
  int8_t previousBackground = -1;
  const char* cheer = "Ready, steady, GO!";
  (void)cheer;
  inputs.gate.reset();
  const uint32_t began = millis();
  while (!game::elapsed(millis(), began, game::speedDurationMs)) {
    uint8_t colour;
    do { colour = random(game::colourCount); } while (colour == previous);
    previous = colour;
    // Word Rush backgrounds must change too, even though the word already changes.
    uint8_t options[game::colourCount];
    uint8_t count = 0;
    for (uint8_t candidate = 0; candidate < game::colourCount; ++candidate)
      if (candidate != colour && candidate != previousBackground) options[count++] = candidate;
    const uint8_t background = options[random(count)];
    previousBackground = background;
    speedAnswer = timeout;
    pollSpeedInput();
    SCREEN(speedCard(word, colour, background, cheer, attempts + 1, pollSpeedInput));
    (void)background;
    showColour(colour);
    const uint32_t used = static_cast<uint32_t>(millis() - began);
    if (used >= game::speedDurationMs) break;
    int8_t answer = timeout;
    uint32_t lastSeconds = 0;
    while (!game::elapsed(millis(), began, game::speedDurationMs)) {
      const uint32_t seconds = (game::speedDurationMs - (millis() - began) + 999) / 1000;
      if (seconds != lastSeconds) {
        SCREEN(speedStatus(score, seconds, streak, cheatEnabled, pollSpeedInput));
        lastSeconds = seconds;
      }
      pollSpeedInput();
      if (speedAnswer != timeout) { answer = speedAnswer; break; }
      delay(1);
    }
    if (answer == cancelled) return;
    if (answer == timeout) break;
    ++attempts;
    if (answer == colour) {
      ++hits;
      score += cheatEnabled ? 2 : 1;
      ++streak;
      cheer = streak >= 3 ? "FAST FINGERS!" : "NICE HIT!";
      gameDisplay.println(cheatEnabled ? F("HACK HIT! +2") : F("Nice! +1"));
      if (streak % 5 == 0) {
        score += cheatEnabled ? 4 : 2;
        cheer = cheatEnabled ? "HACK COMBO! +2 and +4 BONUS!" : "ON FIRE! +1 and +2 BONUS!";
        gameDisplay.println(cheatEnabled ? F("HACK COMBO! Streak bonus +4!") : F("ON FIRE! Streak bonus +2!"));
      }
    } else {
      streak = 0;
      cheer = "Oops! New streak, let us GO!";
      gameDisplay.println(F("Oops! Streak reset. Keep going!"));
    }
  }
  gameDisplay.println(F("TIME!"));
  gameDisplay.print(F("Score: ")); gameDisplay.println(score);
  gameDisplay.print(F("Correct / attempts: ")); gameDisplay.print(hits);
  gameDisplay.print(F(" / ")); gameDisplay.println(attempts);
  const uint32_t record = word ? best : bestSpeed;
  if (!cheatEnabled && score > record) {
    if (word) best = score;
    else bestSpeed = score;
    saveRecord(word ? game::Mode::Word : game::Mode::Speed);
    gameDisplay.println(F("NEW RECORD!"));
  }
  gameDisplay.print(F("Best: ")); gameDisplay.println(word ? best : bestSpeed);
  SCREEN(results(true, score, word ? best : bestSpeed, "", game::maxMemoryLevel, attempts, hits, !cheatEnabled && score > record, cheatEnabled));
  waitForResults();
}

bool showSequence(uint32_t seed, uint32_t level, bool word = false) {
  gameDisplay.print(word ? F("WORD MEMORY - Level ") : F("MEMORY QUEST - Level ")); gameDisplay.println(level);
  gameDisplay.print(F("Colours: ")); gameDisplay.print(game::sequenceLength(level));
  gameDisplay.println(F(" | One chance"));
  gameDisplay.print(F("Time per answer: ")); gameDisplay.print((word ? game::wordAnswerTime(level) : game::answerTime(level)));
  gameDisplay.println(F(" ms"));
  gameDisplay.println(F("WATCH! Hands off the buttons."));
  SCREEN(memory(level, "WATCH"));
  if (!pauseFor(game::memoryWatchPauseMs)) return false;
  game::Sequence sequence(seed);
  for (uint32_t i = 0; i < game::sequenceLength(level); ++i) {
    gameDisplay.print(F("WATCH - ")); gameDisplay.print(i + 1);
    gameDisplay.print('/'); gameDisplay.println(game::sequenceLength(level));
    const uint8_t color = sequence.next();
    if (word) {
      game::Sequence distractions(seed ^ (0x9E3779B9u + i));
      const uint8_t background = (color + 1 + distractions.next() % 3) % game::colourCount;
      SCREEN(wordCard(color, background));
      (void)background;
    } else {
      SCREEN(watch(color));
    }
    showColour(color);
    gameDisplay.flush();  // Start the visible interval after transmission finishes.
    if (!pauseFor(word ? game::wordShowTime(level) : game::showTime(level))) return false;
    SCREEN(gap());
    gameDisplay.flush();
    if (!pauseFor(game::gapTime(level))) return false;
  }
  return true;
}

// Discard playback gestures, including a button held across the phase boundary.
// The answer timer starts only once every color is released.
bool prepareMemoryTurn() {
  inputs.gate.reset();
  bool prompted = false;
  for (;;) {
    inputs.poll();
    if (inputs.start.pressed) return false;
    bool held = false;
    for (uint8_t i = 0; i < game::colourCount; ++i) {
      held |= inputs.colours[i].down() || digitalRead(hardware::colourPins[i]) == LOW;
    }
    if (!held) {
      inputs.gate.reset();
      // Arm from the released state; the next fresh gesture is the first answer.
      inputs.gate.update(0);
      inputs.answer = timeout;
      return true;
    }
    if (!prompted) {
      SCREEN(releaseForTurn());
      gameDisplay.println(F("Sequence finished. Release all buttons. Early presses ignored."));
      prompted = true;
    }
    delay(1);
  }
}

void memoryGame(bool word = false) {
  const uint8_t finalLevel = word ? game::maxWordLevel : game::maxMemoryLevel;
  uint32_t& best = word ? bestWord : bestMemory;
  const uint32_t previousBest = best;
  (void)previousBest;
  gameDisplay.println(word ? F("WORD MEMORY: remember the WORD, ignore the background!") : F("MEMORY QUEST"));
  gameDisplay.println(F("Watch, then repeat the colours in order. Start with 1 colour."));
  gameDisplay.println(word ? F("10 levels: 1 through 10 words. Answer with colour buttons.") : F("10 levels, each with one more colour, all freshly randomized."));
  gameDisplay.println(F("One wrong answer or timeout ends the game."));
  gameDisplay.println(F("Press one button at a time; release before the next answer."));
  if (!countdown()) return;

  uint32_t seed = static_cast<uint32_t>(random(1, 0x7fffffff));
  const char* reason = "";
  (void)reason;
  uint32_t completed = 0;
  uint32_t attempts = 0, hits = 0;
  for (uint32_t level = 1; level <= finalLevel; ++level) {
    if (!showSequence(seed, level, word)) return;
    if (!prepareMemoryTurn()) return;
    gameDisplay.println(F("YOUR TURN! Repeat the colours now. Early presses were ignored."));
    bool success = true;

    game::Sequence sequence(seed);
    for (uint32_t i = 0; i < game::sequenceLength(level); ++i) {
      gameDisplay.print(F("Level ")); gameDisplay.println(level);

      gameDisplay.println(F("YOUR TURN"));
      gameDisplay.print(F("Answer ")); gameDisplay.print(i + 1);
      gameDisplay.print('/'); gameDisplay.println(game::sequenceLength(level));
      SCREEN(memory(level, "YOUR TURN", i + 1));
      const int8_t answer = waitForAnswer((word ? game::wordAnswerTime(level) : game::answerTime(level)) * (cheatEnabled ? 2u : 1u), true);
      if (answer == cancelled) return;
      if (answer != timeout) ++attempts;
      const uint8_t expected = sequence.next();
      if (answer != expected) {
        success = false;
        reason = answer == timeout ? "Time ran out" : answer == -2 ? "One button at a time" : "Wrong color";
        if (answer == timeout) gameDisplay.println(F("Time ran out!"));
        else if (answer == -2) gameDisplay.println(F("One colour at a time!"));
        else gameDisplay.println(F("Oops, that colour slipped away!"));
        gameDisplay.print(F("Expected: ")); gameDisplay.println(hardware::colourName(expected));
        break;
      }
      ++hits;
      gameDisplay.println(F("Yes!"));
    }
    if (!success) break;
    completed = level;
    if (!cheatEnabled && completed > best) {
      best = completed;
      saveRecord(word ? game::Mode::Word : game::Mode::Memory);
    }
    gameDisplay.println(F("LEVEL CLEARED!"));
    SCREEN(feedback(true, completed, "Well done!"));
    if (level == finalLevel) {
      reason = word ? "All 10 levels cleared!" : "All 10 levels cleared!";
      break;
    }
    if (!pauseFor(game::memoryCelebrationMs)) return;
    seed = static_cast<uint32_t>(random(1, 0x7fffffff));
  }
  if (completed == finalLevel) gameDisplay.println(word ? F("QUEST COMPLETE! All 10 levels cleared!") : F("QUEST COMPLETE! All 10 levels cleared!"));
  else gameDisplay.println(F("QUEST OVER! Ready for a comeback?"));
  gameDisplay.print(F("Levels completed: ")); gameDisplay.println(completed);
  gameDisplay.print(F("Best: ")); gameDisplay.println(best);
  gameDisplay.print(F("Attempts: ")); gameDisplay.println(attempts);
  gameDisplay.print(F("Correct: ")); gameDisplay.println(hits);
  SCREEN(results(false, completed, best, reason, finalLevel, attempts, hits, !cheatEnabled && completed > previousBest, cheatEnabled));
  waitForResults();
}

void showMenu() {
  gameDisplay.println(F("RUNTIME"));
  gameDisplay.println(selectedMode == game::Mode::Speed ? F("Selected: SPEED SPRINT") : selectedMode == game::Mode::Word ? F("Selected: WORD RUSH") : F("Selected: MEMORY QUEST"));
  gameDisplay.print(F("Best sprint: ")); gameDisplay.print(bestSpeed);
  gameDisplay.print(F(" | Memory: ")); gameDisplay.println(bestMemory);
  gameDisplay.println(F("Flip MODE: next game (Memory -> Speed -> Word Rush -> Memory)"));
  gameDisplay.println(F("START: play / quit | RED on menu: how to play"));
  SCREEN(menu(selectedMode, bestSpeed, bestMemory, bestWord, cheatEnabled));
  if (cheatEnabled) gameDisplay.println(F("HACK MODE: double answer time / double sprint points. Records disabled."));
}
}  // namespace

void setup() {
  inputs.begin();
  gameDisplay.begin(hardware::baudRate);
  loadRecords();
  showMenu();
}

void loop() {
  static bool startPending = false;
  inputs.poll();
  if (inputs.mode.pressed || inputs.mode.released) {
    cheatCode.reset();
    selectedMode = game::nextMode(selectedMode);
    showMenu();
  }
  if (cheatCode.enter(inputs.answer, millis())) {
    cheatEnabled = !cheatEnabled;
    gameDisplay.println(cheatEnabled ? F("SECRET UNLOCKED! HACK MODE ON") : F("HACK MODE OFF"));
    showMenu();
  }
  bool playFromHelp = false;
  if (inputs.answer == 0) {
    startPending = false;
    cheatCode.reset();
    playFromHelp = howToPlay(selectedMode);
    if (!playFromHelp) { showMenu(); return; }
  }
  if (inputs.start.pressed) startPending = true;
  if (playFromHelp || (startPending && inputs.start.released)) {
    startPending = false;
    cheatCode.reset();
#ifdef ARDUINO_ARCH_ESP32
    randomSeed(esp_random());
#else
    randomSeed(micros());
#endif
    if (selectedMode == game::Mode::Speed) speedGame();
    else if (selectedMode == game::Mode::Word) speedGame(true);
    else memoryGame();
    showMenu();
  }
  delay(1);
}
