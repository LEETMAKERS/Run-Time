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

  assert(game::sequenceLength(1) == 3);
  assert(game::sequenceLength(game::maxLevel) == game::maxLevel + 2);
  for (uint8_t level = 2; level <= game::maxLevel; ++level) {
    assert(game::sequenceLength(level) > game::sequenceLength(level - 1));
    assert(game::showTime(level) < game::showTime(level - 1));
    assert(game::answerTime(level) < game::answerTime(level - 1));
    assert(game::gapTime(level) < game::gapTime(level - 1));
  }
  assert(!game::elapsed(9, UINT32_MAX - 10, 21));
  assert(game::elapsed(10, UINT32_MAX - 10, 21));
  button.begin(false, UINT32_MAX - 20);
  button.update(true, UINT32_MAX - 10);
  button.update(true, 14);
  assert(button.pressed);
  puts("PASS: debounce, hold/release, chords, difficulty, timer rollover");
}
