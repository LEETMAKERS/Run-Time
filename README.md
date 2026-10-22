# Run Time

A three-game ESP32 arcade with four colour buttons: **Red, Blue, Green, and Brown**.
Train your memory in **Memory Quest**, race in **Speed Sprint**, or beat the
colour distraction in **Word Rush**.

## Controls

| Screen | Control | Action |
| --- | --- | --- |
| Menu | MODE | Select the next game |
| Menu | START, press and release | Play immediately |
| Menu | RED | Open optional How to play instructions |
| How to play | START, press and release | Begin the countdown |
| How to play | MODE | Return to the menu |
| Playing | Colour buttons | Answer; release all colours between answers |
| Playing / countdown | START | Cancel and return to the menu |
| Results | START, press and release | Return to the menu |
| Results | MODE | Return to the menu |

The menu starts on Memory Quest and cycles **Memory Quest → Speed Sprint →
Word Rush → Memory Quest**. Both directions of the MODE switch count; holding
it does not repeat. MODE changes during gameplay are ignored.
Instructions are optional, so experienced players can start straight away.

## Memory Quest

Watch the sequence, then repeat it in order after **YOUR TURN - PRESS NOW**.
For example, **Red → Brown → Blue** means press Red, Brown, then Blue,
releasing each button before the next press.

- Clear **10 levels**, starting with one colour and adding one per level.
- Each level has a freshly randomized sequence. The same colour can appear at most **twice in a row**, never three times.
- Playback shows full-screen colours separated by black gaps. Colour presses
  during playback are ignored, including presses in the gaps.
- If a colour is held when playback ends, release all colour buttons first.
  The first answer deadline starts after release; early answers are not queued.
- During YOUR TURN, a **live countdown** resets for every answer and turns red
  in the final second. **ANSWER IN TIME OR YOU LOSE!** stays visible.
- One wrong answer, multiple colours pressed together, or a timeout ends the game.
  There are no lives or retries within a round.

| Timing | Level 1 | Level 10 |
| --- | --- | --- |
| Each displayed colour | 650 ms | 380 ms |
| Gap between colours | 180 ms | 108 ms |
| Time per answer | 4 seconds | 3.325 seconds |

The WATCH prompt lasts 700 ms. Completed levels get a 350 ms celebration,
with special milestones at levels **3, 5, and 8**. Clear level 10 to win.
Losing shows a silly animated face.

## Speed Sprint

You have **15 seconds** to press the button matching each displayed colour.
The next target appears after each answer and always uses a different colour.
A target counter makes each new prompt visible; release all buttons before answering it.

- Correct answer: **+1 point**.
- Every **5 consecutive correct answers**: **+2 bonus points**.
- Wrong or multiple-button answers: no points; the streak resets.
- Points already earned are kept. Release all colour buttons between answers.

The screen shows your score, live timer, time bar, combo meter, and how many
hits remain until your next bonus. Live cheers celebrate hits and streaks.

## Word Rush

The third game is a **15-second speed challenge**. Match the large **word**
and ignore the different-coloured background: **RED on blue means press Red**.
The word appears directly on the coloured background, without a black card.
Both the word and background change for each new target, so consecutive prompts
never look stuck on the same colour.

Scoring, streak bonuses, and controls work like Speed Sprint. Word Rush keeps
its own saved best score.

## Results and records

Results show **Attempts**, **Correct**, your total score or levels cleared,
and the saved best. Attempts count fresh answer gestures, including wrong
and multiple-button answers. Playback presses and timeouts do not count.
Correct includes successful answers in an unfinished Memory level.

Speed results show separate rows for **Correct × points per hit**, **Streak bonus**,
and **TOTAL POINTS**, followed by correct answers out of attempts, accuracy, and
a gold record panel with the saved best and the next score to aim for. Hack mode explicitly shows two points per correct hit.
For example, **17 correct out of 18 attempts**, with two streak bonuses, means
**17 + 4 = 21 points**.

The menu uses cyan, gold, and purple game cards. A large gold **RECORD TO BEAT**
panel shows the selected game’s saved record, labelled in points or levels. New records get a
**NEW RECORD!** heading with confetti; Memory victories also get confetti.
Press and release **START**, or flip **MODE**, to return to the menu.

Records are separate for each game: highest level completed in Memory Quest,
and highest score in each speed game. They are saved in ESP32 flash (NVS) and restored after restart or power-off.
Only improved records are written; Memory records save after each newly best
completed level. Cheat runs do not overwrite them. Erasing the board’s flash
clears saved records.

## Secret cheat code

On the menu, enter **Blue → Green → Brown → Green → Blue**, releasing all
buttons between steps. Enter each step within three seconds of the previous one.
The same code toggles **HACK MODE** off. Red stays reserved for help.
Wrong inputs, multiple buttons, help, changing games, or starting a game reset
an unfinished code. The code is only recognized on the menu.

HACK MODE gives **double answer time** in Memory Quest and **double points**
(including streak bonuses) in both speed games. The menu and results mark cheat
runs; they do not update saved records. It stays enabled until toggled off or
restarted. The normal 15-second speed-game duration stays the same.

## ESP32 setup

Configured for a standard ESP32 DevKit (`esp32dev`), using the Arduino framework.
Wire each button between its GPIO and GND (internal pull-ups are enabled).

| Control | GPIO |
| --- | --- |
| Red | 25 |
| Blue | 26 |
| Green | 27 |
| Brown | 32 |
| Start | 22 |
| Mode switch | 21 |

The menu starts on Memory Quest. Each debounced change of MODE selects the next game; its physical position does not identify the selection. MODE changes during play are ignored.
Use 3.3 V logic. Game prompts and colors appear on the ST7789 TFT and are also logged over USB at 115200 baud.
Run `./run-game.sh build` to compile or `./run-game.sh run` to upload and play.
Run `./run-game.sh buttons` to check the wiring.

### ST7789 display wiring

| TFT pin | ESP32 GPIO |
| --- | --- |
| CS | 5 |
| DC | 16 |
| RESET | 4 |
| SCK / CLK | 18 |
| MOSI / SDI | 23 |
| MISO / SDO (optional) | 19 |

Connect TFT GND to ESP32 GND. Follow your display module's supply and backlight
ratings; its SPI signals use 3.3 V logic.
The root `main.cpp` is the game entry point selected by PlatformIO.

### Screen colors

The display uses the hardware-verified TEST 5 settings: ST7789, 240×320 initialization,
landscape rotation, native RGB565 color order, inversion disabled, and 4 MHz SPI.
Run `./run-game.sh buttons` to compare each labeled color with its button.
If your panel swaps red and blue, add `build_flags = -DTFT_SWAP_RED_BLUE=1`
to `[env:esp32dev]` in `platformio.ini`, then upload again. If black and white
are reversed, use `-DTFT_INVERT_COLORS=1`. Both flags may be combined.

### Identify a screen with wrong colors or an unused strip

The module label confirms 240×320 pixels but does not identify its controller.
Run `./run-game.sh display` to upload the screen diagnostic. Press and release
START to cycle eight profiles: ILI9341 or ST7789, landscape or portrait, and
native or swapped red/blue order. Each profile draws a full-screen white border,
a white square in the bottom-right corner, and labeled red, blue, green, brown,
and white bars. Report the TEST number whose colors and whole-screen border
are correct. The diagnostic also logs ILI9341 ID bytes over USB; readings require
TFT SDO/MISO connected to ESP32 GPIO 19 and may be unavailable on some modules.
The test uses 4 MHz SPI. Run `./run-game.sh run` to restore the game afterward.

The game and diagnostics share the RGB565 button palette in
`include/colour_palette.h`. Adjust color values there if needed for your panel.

### Test all buttons and the screen

Run `./run-game.sh buttons` to upload the combined hardware test.
The screen lists RED, BLUE, GREEN, BROWN, and START independently, so you
can also check simultaneous presses. Each control shows DOWN while held,
UP when released, and OK after a complete press and release. Flip MODE
both ways to check both positions. These checks reset when power is removed.

Check the six screen bars (red, blue, green, brown, white, black) and the
white border on all four edges. Screen correctness must be checked visually.
The test runs from USB power without the IDE after uploading.
For controller/orientation troubleshooting, use `./run-game.sh display`.
Restore the game afterward with `./run-game.sh run`.

### Run automated checks

Run `./run-game.sh test` for host simulations of timing, input handling, random
sequences, and the 10-level victory. Run `./run-game.sh build` to compile the ESP32 firmware.
