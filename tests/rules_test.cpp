#include <assert.h>
#include <stdio.h>
#include "../include/game_rules.h"

int main() {
  game::Button button;
  button.begin(false, 0);
  button.update(true, 1);
  button.update(false, 10);
  button.update(true, 15);
  button.update(true, 39);
  assert(!button.pressed && !button.down());
  button.update(true, 40);
  assert(button.pressed && button.down());
  button.update(true, 5000);
  assert(!button.pressed && button.down());
  button.update(false, 5010);
  button.update(false, 5035);
  assert(button.released && !button.down());

  game::ColourGate gate;
  assert(gate.update(1) == -1); // Held before turn: ignored.
  assert(gate.update(0) == -1);
  assert(gate.update(1) == 0);
  assert(gate.update(1) == -1);
  assert(gate.update(2) == -1); // No release: cannot answer again.
  gate.update(0);
  assert(gate.update(3) == -2); // Chord is invalid.
  assert(gate.update(2) == -1);
  gate.update(0);
  assert(gate.update(8) == 3);
  gate.reset();
  assert(gate.update(8) == -1);

  assert(game::sequenceLength(1) == 1);
  assert(game::sequenceLength(300) == 300);
  for (uint8_t level = 2; level <= game::maxMemoryLevel; ++level) {
    assert(game::sequenceLength(level) > game::sequenceLength(level - 1));
    assert(game::showTime(level) < game::showTime(level - 1));
    assert(game::answerTime(level) < game::answerTime(level - 1));
    assert(game::gapTime(level) < game::gapTime(level - 1));
  }
  assert(game::maxWordLevel == 10);
  assert(game::wordShowTime(1) == 600 && game::wordShowTime(10) == 240);
  assert(game::wordAnswerTime(1) == 2500 && game::wordAnswerTime(10) == 1000);
  assert(game::wordShowTime(1000000) == 240);
  assert(game::wordAnswerTime(1000000) == 1000);
  for (uint8_t level = 2; level <= game::maxWordLevel; ++level) {
    assert(game::wordShowTime(level) < game::wordShowTime(level - 1));
    assert(game::wordAnswerTime(level) < game::wordAnswerTime(level - 1));
  }
  assert(game::maxMemoryLevel == 10);
  assert(game::showTime(1) == 650 && game::showTime(10) == 380);
  assert(game::gapTime(1) == 180 && game::gapTime(10) == 108);
  // Seed 4 used to begin with four reds. Keep the pair, prevent the third.
  game::Sequence repeated(4);
  assert(repeated.next() == 0);
  assert(repeated.next() == 0);
  assert(repeated.next() != 0);
  for (uint32_t seed = 0; seed < 1000; ++seed) {
    game::Sequence stream(seed), replay(seed);
    uint8_t previous = game::colourCount;
    unsigned run = 0;
    for (unsigned i = 0; i < 100; ++i) {
      const uint8_t colour = stream.next();
      assert(colour < game::colourCount);
      assert(colour == replay.next());
      run = colour == previous ? run + 1 : 1;
      assert(run <= 2);
      previous = colour;
    }
  }
  game::Sequence first(123), retry(123), fresh(456);
  bool differs = false;
  for (unsigned i = 0; i < 300; ++i) {
    const auto colour = first.next();
    assert(colour < game::colourCount);
    assert(colour == retry.next());
    differs |= colour != fresh.next();
  }
  assert(differs);
  assert(game::showTime(1000000) == 220);
  assert(game::answerTime(1000000) == 1075);
  assert(!game::elapsed(9, UINT32_MAX - 10, 21));
  assert(game::elapsed(10, UINT32_MAX - 10, 21));
  button.begin(false, UINT32_MAX - 20);
  button.update(true, UINT32_MAX - 10);
  button.update(true, 14);
  assert(button.pressed);
  game::CheatCode secret;
  const int8_t code[] = {1, 2, 3, 2, 1};
  for (unsigned pass = 0; pass < 2; ++pass)
    for (unsigned i = 0; i < 5; ++i)
      assert(secret.enter(code[i], pass * 1000 + i * 100) == (i == 4));
  assert(!secret.enter(1, 2000));
  assert(!secret.enter(2, 5000)); // Three seconds between steps expires it.
  assert(!secret.enter(1, 5100));
  assert(!secret.enter(-2, 5200)); // Chords break the code.
  assert(!secret.enter(2, 5300));
  assert(!secret.enter(3, 5400));
  assert(!secret.enter(2, 5500));
  assert(!secret.enter(1, 5600));
  secret.reset();
  assert(!secret.enter(1, 6000));
  assert(!secret.enter(0, 6100)); // RED cannot complete a secret.
  puts("PASS: debounce, hold/release, chords, difficulty, timer rollover");
}
