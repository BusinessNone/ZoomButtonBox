# ZoomButtonBox

Firmware for a small physical **Zoom button box** — a handful of momentary
push buttons wired to an Arduino that plugs into your computer over USB and
appears as a keyboard. Each button fires a Zoom keyboard shortcut, so you can
mute, raise your hand, toggle video, or go full screen without hunting for the
on-screen controls.

---

## Hardware

| Requirement | Notes |
|-------------|-------|
| **Board** | An Arduino with native USB HID — ATmega32U4-based, e.g. **Leonardo, Micro, or Pro Micro**. (An Uno/Nano will *not* work; their USB chip can't act as a keyboard.) |
| **Buttons** | 4 momentary push buttons, each wired between a digital pin and **GND**. No external resistors needed — the firmware uses the chip's internal pull-ups. |
| **LEDs** | Optional. The code supports a per-button indicator LED but ships with all of them disabled, so the box works fine without any. |

### Default pin wiring

| Function | Pin | Zoom shortcut sent |
|----------|-----|--------------------|
| Mute / unmute audio | `1` | `Alt + A` |
| Raise / lower hand | `2` | `Alt + Y` |
| Start / stop video | `6` | `Alt + V` |
| Enter / exit full screen | `8` | `Alt + F` |

> The shortcuts are the **Windows** Zoom defaults. On macOS the key
> combinations differ, so you'd change the keys in the sketch (see below).

---

## Software setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the **HID-Project** library (Library Manager → search
   "HID-Project" by NicoHood). This provides `BootKeyboard`.
3. Open [`ZoomButtonsNew.ino`](./ZoomButtonsNew.ino).
4. Select your board (e.g. *Arduino Leonardo*) and the right port.
5. Upload. The box now enumerates as a USB keyboard.

Open the Serial Monitor at **9600 baud** to see debug output (startup button
states and a line each time a button is pressed).

---

## How the logic works

### 1. USB HID boot keyboard

The sketch includes `HID-Project.h` and uses `BootKeyboard`. A *boot*
keyboard is the simplest HID keyboard profile — the one a PC recognizes at
the BIOS/login screen — which makes the device work reliably the instant it's
plugged in. `BootKeyboard.begin()` in `setup()` starts it.

### 2. Configuration via `#define`

All wiring lives in `#define`s at the top so nothing is hard-coded deeper in
the logic:

```cpp
#define NUMBER_OF_BUTTONS 4
#define OFF_DELAY 180000   // LED auto-off timeout: 3 minutes

#define BTN_MUTE        1
#define BTN_RAISEHAND   2
#define BTN_VIDEOTOGGLE 6
#define BTN_FULLSCREEN  8

#define NO_LED 99          // sentinel: "this button has no LED"
```

`NO_LED` is a magic pin value meaning "no LED here". The LED helpers ignore
it, so LEDs are entirely optional without sprinkling `if (hasLed)` checks
everywhere.

### 3. The arrays that drive the loop

```cpp
int buttons[NUMBER_OF_BUTTONS] = {BTN_RAISEHAND, BTN_VIDEOTOGGLE, BTN_FULLSCREEN, BTN_MUTE};
int leds[NUMBER_OF_BUTTONS]    = {LED_RAISEHAND, LED_VIDEOTOGGLE, LED_MUTE, LED_FULLSCREEN};
uint32_t ledsOnUntil[NUMBER_OF_BUTTONS];
```

- `buttons[]` — the pins the main loop scans, in order.
- `leds[]` — the LED pin paired with each button (all `NO_LED` by default).
- `ledsOnUntil[]` — per-button "turn the LED off at this `millis()` time";
  `0` means the LED is off / no timer running.

### 4. `setup()`

Runs once at power-up:

1. Starts serial at 9600 baud.
2. **First pass:** lights each LED and sets each button pin to
   `INPUT_PULLUP`, with a 100 ms gap so the LEDs "walk" as a quick self-test.
3. **Second pass:** turns the LEDs back off, clears each auto-off timer, and
   prints the resting state of every button over serial (`1` = released).
4. Calls `BootKeyboard.begin()` to go live as a keyboard.

### 5. `loop()` — the scan cycle

For every button, on every pass:

**a. LED auto-off.** If a button's timer (`ledsOnUntil[i]`) is running and its
deadline has passed (`< millis()`), turn the LED off and clear the timer. This
is what makes a lit LED extinguish itself after `OFF_DELAY` (3 min).

**b. Press detection.** With `INPUT_PULLUP`, a pin reads **LOW** while its
button is held (the button connects the pin to ground). When a press is
detected:

- **Toggle the LED timer** — if the LED was already on, turn it off; otherwise
  light it and arm the `OFF_DELAY` countdown.
- **Send the shortcut** via a `switch` on the button's *pin number* (not the
  loop index — keying on the pin keeps each case readable):

  ```cpp
  case BTN_MUTE:
    BootKeyboard.press(KEY_LEFT_ALT);   // hold Alt
    delay(50);
    BootKeyboard.write(KEY_A);          // tap A
    delay(100);
    BootKeyboard.release(KEY_LEFT_ALT); // release Alt
    break;
  ```

  Every case follows the same *press-modifier → tap-key → release-modifier*
  pattern, with small `delay()`s so the host reliably registers the
  combination. An unmapped pin hits `default` and just logs "Unknown case".

**c. Debounce / one-press-one-action.** After sending, the code blocks in a
tight `while (digitalRead(...) == LOW) delay(10);` loop until the button is
released. This both debounces the switch and guarantees a single press sends
exactly one shortcut, no matter how long it's held.

---

## Customizing

- **Different shortcut:** change the `KEY_*` value inside the relevant
  `case` (e.g. swap `KEY_A` for the key your app uses).
- **Different / more buttons:** bump `NUMBER_OF_BUTTONS`, add a `BTN_*`
  define, add it to `buttons[]`, and add a matching `case` in the `switch`.
- **Enable LEDs:** point the `LED_*` defines at real output pins (instead of
  `NO_LED`) and make sure the `leds[]` order lines up with `buttons[]`.
- **LED timeout:** change `OFF_DELAY` (milliseconds).

### Known rough edges

- **`leds[]` is not index-aligned with `buttons[]`.** Index 2's button is
  `BTN_FULLSCREEN` but its LED entry is `LED_MUTE`, and index 3 is the
  reverse. This is harmless today because every LED is `NO_LED`, but the array
  should be re-ordered before wiring real LEDs.
- **`BTN_GALLERVIEW` / `BTN_SPEAKERVIEW` / `BTN_MUTEALL`** are defined as
  placeholder pin `10` but aren't in `buttons[]` or the `switch` — they're
  stubs for features that aren't implemented yet.

---

## Source

The complete firmware is a single sketch:
**[`ZoomButtonsNew.ino`](./ZoomButtonsNew.ino)** (fully commented).

## License

See [`LICENSE`](./LICENSE).
