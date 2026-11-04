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
| **Board** | A **Seeed Studio XIAO (SAMD21)** on a **Grove Shield for Seeeduino XIAO**. It has native USB, so it can act as a keyboard. |
| **Buttons** | 4 **Grove Red LED Button** modules. Each has a button and a red LED. |

### How the Grove LED Button works

Each module has two signals on its Grove cable. The yellow wire (SIG1) drives
the LED (HIGH = on). The white wire (SIG2) is the button, pulled up on the
module and pulled LOW when pressed. On a shield connector labelled `Dn`, SIG1
is pin `Dn` and SIG2 is pin `D(n+1)`.

### Default wiring

Pin numbers are Arduino pin numbers, so `3` is XIAO pin D3. The four connectors
below are the ones that don't share pins.

| Function | Shield connector | LED pin | Button pin | Zoom shortcut sent |
|----------|------------------|---------|------------|--------------------|
| Mute / unmute audio | D0 | `0` | `1` | `Alt + A` |
| Raise / lower hand | D2 | `2` | `3` | `Alt + Y` |
| Start / stop video | I2C (SDA 4 / SCL 5) | `5` | `4` | `Alt + V` |
| Enter / exit full screen | UART (TX 6 / RX 7) | `7` | `6` | `Alt + F` |

> The I2C and UART connectors are used as plain digital pins here, which is
> fine as long as nothing else uses I2C or the hardware UART.
>
> The XIAO's pins are 3.3V only. The Grove modules accept 3.3V or 5V, so
> power them from the shield's 3V3 pins.
>
> The shortcuts are the **Windows** Zoom defaults. On macOS the key
> combinations differ, so you'd change the keys in the sketch (see below).

---

## Software setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. In File > Preferences, add this URL to "Additional boards manager URLs":
   `https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json`
3. Install **Seeed SAMD Boards** from the Boards Manager, then select
   **Seeed Studio XIAO** under Tools > Board.
4. Install the **Keyboard** library (Library Manager, by Arduino).
5. Open [`ZoomButtonsNew.ino`](./ZoomButtonsNew.ino) and upload. The box now
   enumerates as a USB keyboard.

CI compiles the sketch for the XIAO on every push.

Open the Serial Monitor at **9600 baud** to see debug output (startup button
states and a line each time a button is pressed).

---

## How the logic works

### 1. USB HID keyboard

The sketch includes `Keyboard.h`, the standard Arduino keyboard library, which
the XIAO's native USB turns into a real keyboard. `Keyboard.begin()` in
`setup()` starts it.

### 2. Configuration via `#define`

Pins, timing and the LED sentinel live in `#define`s at the top:

```cpp
#define OFF_DELAY   180000UL  // LED auto-off timeout: 3 minutes
#define DEBOUNCE_MS 30        // a pin must be stable this long to count

#define BTN_MUTE        1
#define BTN_RAISEHAND   3
#define BTN_VIDEOTOGGLE 4
#define BTN_FULLSCREEN  6

#define NO_LED 99             // sentinel: "this button has no LED"
```

`NO_LED` is a magic pin value meaning "no LED here". The LED helpers ignore
it, so LEDs are entirely optional without sprinkling `if (hasLed)` checks
everywhere.

### 3. One table drives everything

```cpp
struct Button { uint8_t pin; uint8_t ledPin; char key; bool remuteOnTimeout; bool holdToResync; };

const Button buttons[] = {
  { BTN_RAISEHAND,   LED_RAISEHAND,   'y', false, false },
  { BTN_VIDEOTOGGLE, LED_VIDEOTOGGLE, 'v', false, false },
  { BTN_FULLSCREEN,  LED_FULLSCREEN,  'f', false, false },
  { BTN_MUTE,        LED_MUTE,        'a', true,  true  },
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
4. Calls `Keyboard.begin()` to go live as a keyboard.

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
- **Auto re-mute** - for the Mute button, a lit LED means "unmuted". If it
  isn't pressed again within `OFF_DELAY` (3 minutes), the sketch sends
  `Alt + A` once more to put you back on mute.
- **Starts muted, with a resync hold** - the box can't read Zoom's real mute
  state (USB HID is one-way), so it assumes it starts muted: the first press
  unmutes you. Sending `Alt + A` at startup would flip Zoom instead, so the
  box doesn't. If the box and Zoom ever disagree, **hold Mute for 2 seconds**
  while Zoom is muted. The LED blinks three times and the box is back in sync
  without sending any keystroke. Mute fires on release so a hold can be told
  apart from a tap.
  To avoid the problem at the source, turn on Zoom's *Mute my microphone when
  joining a meeting* setting.
- **Send the shortcut** with `sendAltShortcut(key)`: press Alt, wait 50 ms, tap
  the key, wait 100 ms, release Alt. The delays give the host time to register
  the combination.

---

## Customizing

- **Different shortcut:** change the key character in that button's row of
  `buttons[]` (e.g. swap `'a'` for the key your app uses).
- **Different / more buttons:** add a `BTN_*` define (and an `LED_*` define)
  and add one row to `buttons[]`. Nothing else needs to change.
- **Disable an LED:** set its `LED_*` define to `NO_LED`.
- **Auto re-mute / LED timeout:** change `OFF_DELAY` (milliseconds). To
  turn auto re-mute off, set the Mute row's last field in `buttons[]` to `false`.

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
