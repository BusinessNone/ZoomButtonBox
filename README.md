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

Pins, timing and the LED sentinel live in `#define`s at the top:

```cpp
#define OFF_DELAY   180000UL  // LED auto-off timeout: 3 minutes
#define DEBOUNCE_MS 30        // a pin must be stable this long to count

#define BTN_MUTE        1
#define BTN_RAISEHAND   2
#define BTN_VIDEOTOGGLE 6
#define BTN_FULLSCREEN  8

#define NO_LED 99             // sentinel: "this button has no LED"
```

`NO_LED` is a magic pin value meaning "no LED here". The LED helpers ignore
it, so LEDs are entirely optional without sprinkling `if (hasLed)` checks
everywhere.

### 3. One table drives everything

```cpp
struct Button { uint8_t pin; uint8_t ledPin; KeyboardKeycode key; };

const Button buttons[] = {
  { BTN_RAISEHAND,   LED_RAISEHAND,   KEY_Y },
  { BTN_VIDEOTOGGLE, LED_VIDEOTOGGLE, KEY_V },
  { BTN_FULLSCREEN,  LED_FULLSCREEN,  KEY_F },
  { BTN_MUTE,        LED_MUTE,        KEY_A },
};
```

Each row holds a button's pin, its LED pin and the key sent with Alt, so the
three can't drift out of step. `NUMBER_OF_BUTTONS` is computed from the table.
Small parallel arrays hold the runtime state: debounced pressed state, last raw
reading and its timestamp, whether the LED timer is running, and when it expires.

### 4. `setup()`

Runs once at power-up:

1. Starts serial at 9600 baud.
2. **First pass:** lights each LED (configured as an output) and sets each
   button pin to `INPUT_PULLUP`, with a 100 ms gap so the LEDs "walk" as a
   quick self-test.
3. **Second pass:** turns the LEDs back off, clears the timers, records the
   resting state of every button (a button held at boot must be released
   before it can fire) and prints it over serial (`1` = released).
4. Calls `BootKeyboard.begin()` to go live as a keyboard.

### 5. `loop()` - the scan cycle

The loop never blocks waiting for a button, so every button and LED timer is
serviced on every pass. For each button:

**a. LED auto-off.** If the LED timer is running and its deadline has passed,
turn the LED off. The comparison uses `(int32_t)(now - deadline) >= 0`, which
stays correct when `millis()` wraps after about 49.7 days.

**b. Debounced press detection.** With `INPUT_PULLUP`, a pin reads **LOW**
while its button is held. A change only counts once the raw reading has been
stable for `DEBOUNCE_MS`, so contact bounce on press or release is ignored.
The shortcut fires once, on the press edge, no matter how long the button is
held.

**c. On press** (`onPress()`):

- **Toggle the LED** - if it was lit, turn it off; otherwise light it and arm
  the `OFF_DELAY` countdown. The LED tracks button presses, not Zoom's real
  state (HID is one-way), so it can drift if you change Zoom another way.
- **Send the shortcut** with `sendAltShortcut(key)`: press Alt, wait 50 ms, tap
  the key, wait 100 ms, release Alt. The delays give the host time to register
  the combination.

---

## Customizing

- **Different shortcut:** change the `KEY_*` value in that button's row of
  `buttons[]` (e.g. swap `KEY_A` for the key your app uses).
- **Different / more buttons:** add a `BTN_*` define (and an `LED_*` define)
  and add one row to `buttons[]`. Nothing else needs to change.
- **Enable LEDs:** point the `LED_*` defines at real output pins (instead of
  `NO_LED`).
- **LED timeout:** change `OFF_DELAY` (milliseconds).

### Not implemented yet

Gallery view, speaker view, mute-all and share were stubbed in earlier
versions (all on a placeholder pin 10). They are left as commented-out
defines in the sketch until they get real pins and `buttons[]` rows.

---

## Source

The complete firmware is a single sketch:
**[`ZoomButtonsNew.ino`](./ZoomButtonsNew.ino)** (fully commented).

## License

See [`LICENSE`](./LICENSE).
