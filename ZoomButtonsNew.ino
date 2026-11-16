/*
 * ZoomButtonsNew.ino
 * ------------------------------------------------------------------------
 * Firmware for a small physical "Zoom button box" built on a Seeed Studio
 * XIAO (ATSAMD21, native USB). Each momentary push button on the box triggers a Zoom
 * keyboard shortcut, so a meeting can be controlled without touching the
 * on-screen controls.
 *
 * How it works
 *   - The board enumerates on the host computer as a USB keyboard via the
 *     standard Arduino Keyboard library.
 *   - Buttons are wired between a digital pin and ground and read with the
 *     internal pull-up resistor (INPUT_PULLUP): the pin reads HIGH when the
 *     button is open and LOW when pressed.
 *   - When a button is pressed the matching Zoom shortcut (Alt + a letter)
 *     is typed onto the host. An optional per-button LED can be lit to give
 *     visual feedback; the LED auto-extinguishes after OFF_DELAY, and the
 *     Mute button also re-sends Alt + A then to put the user back on mute.
 *   - The box cannot read Zoom's real mute state (USB HID is one-way). It
 *     assumes it starts muted. If it ever disagrees with Zoom, hold Mute for
 *     2 seconds to tell it "Zoom is muted now" (no keystroke is sent).
 *
 * Zoom shortcuts sent (Windows defaults):
 *   Mute/unmute audio ......... Alt + A
 *   Raise / lower hand ........ Alt + Y
 *   Start / stop video ........ Alt + V
 *   Enter / exit full screen .. Alt + F
 *
 * Wiring notes
 *   - Each button connects its pin to GND; no external resistor is needed
 *     because INPUT_PULLUP is used.
 *   - LEDs are optional. A button whose LED pin is NO_LED simply skips
 *     all LED handling, so the box works with or without LEDs installed.
 * ------------------------------------------------------------------------
 */

#include <Keyboard.h>   // Arduino USB HID keyboard (native USB on the XIAO SAMD21)

// How long (in milliseconds) a button's LED stays lit after a press before
// it is automatically turned off. 180000 ms = 3 minutes.
#define OFF_DELAY 180000UL

// A press/release is accepted only after the pin has been stable this long.
#define DEBOUNCE_MS 30

// Holding a resync button (Mute) this long tells the box "Zoom is muted right
// now" WITHOUT sending a keystroke. Use it if the box and Zoom ever disagree.
#define RESYNC_HOLD_MS 2000

// Sentinel pin value meaning "this button has no LED". ledOn()/ledOff()
// ignore any pin equal to NO_LED, so LED wiring is entirely optional.
#define NO_LED 99

// --- Pin mapping ----------------------------------------------------------
// Four Seeed Grove Red LED Button modules on the Grove Shield for XIAO.
// On each module the yellow wire (SIG1) drives the LED (HIGH = on) and the
// white wire (SIG2) is the button (LOW = pressed, pulled up). On a shield
// connector labelled Dn, SIG1 is pin Dn and SIG2 is pin D(n+1).
//
//   Function      Shield connector     LED pin   Button pin
//   Mute          D0                   0         1
//   Raise hand    D2                   2         3
//   Video         I2C (SDA4/SCL5)      5         4
//   Full screen   UART (TX6/RX7)       7         6
#define BTN_MUTE        1
#define BTN_RAISEHAND   3
#define BTN_VIDEOTOGGLE 4
#define BTN_FULLSCREEN  6
// Not implemented yet (left commented out):
//#define BTN_GALLERVIEW
//#define BTN_SPEAKERVIEW
//#define BTN_MUTEALL
//#define BTN_SHARE

// Digital pin driving each button's indicator LED (NO_LED to disable).
#define LED_MUTE        0
#define LED_RAISEHAND   2
#define LED_VIDEOTOGGLE 5
#define LED_FULLSCREEN  7

// One row per button: its pin, its LED pin, and the key sent with Alt.
// Keeping the three together means a button, its LED and its shortcut can
// never get out of step with each other.
struct Button {
  uint8_t pin;
  uint8_t ledPin;
  char key;
  bool remuteOnTimeout;   // resend the shortcut when the OFF_DELAY timer expires
  bool holdToResync;      // hold RESYNC_HOLD_MS to reset to "muted" with no keystroke
};

const Button buttons[] = {
  { BTN_RAISEHAND,   LED_RAISEHAND,   'y', false, false },  // Alt + Y : raise / lower hand
  { BTN_VIDEOTOGGLE, LED_VIDEOTOGGLE, 'v', false, false },  // Alt + V : start / stop video
  { BTN_FULLSCREEN,  LED_FULLSCREEN,  'f', false, false },  // Alt + F : enter / exit full screen
  { BTN_MUTE,        LED_MUTE,        'a', true,  true  },  // Alt + A : mute / unmute audio (auto re-mutes, hold to resync)
};
const uint8_t NUMBER_OF_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// Per-button runtime state (parallel to buttons[]).
bool     stablePressed[NUMBER_OF_BUTTONS];   // debounced state: true = held
bool     lastReading[NUMBER_OF_BUTTONS];     // last raw reading
uint32_t lastChangeMs[NUMBER_OF_BUTTONS];    // when the raw reading last changed
uint32_t pressStartMs[NUMBER_OF_BUTTONS];    // when the current press began
bool     holdHandled[NUMBER_OF_BUTTONS];     // this press was a resync hold (or ignored)
bool     ledLit[NUMBER_OF_BUTTONS];          // LED timer running
uint32_t ledOffAtMs[NUMBER_OF_BUTTONS];      // when to switch the LED off

//=========================================
// LED helpers (no-op for NO_LED)
//=========================================
void ledOn(uint8_t parPin){
  if (parPin!=NO_LED) digitalWrite(parPin, HIGH);
}
void ledOff(uint8_t parPin){
  if (parPin!=NO_LED) digitalWrite(parPin, LOW);
}

// Type Alt+<key> on the host: hold Alt, tap the key, release Alt. The small
// delays give the host time to register the modifier and key.
void sendAltShortcut(char parKey){
  Keyboard.press(KEY_LEFT_ALT);
  delay(50);
  Keyboard.write(parKey);
  delay(100);
  Keyboard.release(KEY_LEFT_ALT);
}

// Called once per debounced press.
void onPress(uint8_t i){
  // Toggle the LED: a press while it is lit turns it off; otherwise light it
  // and arm the OFF_DELAY timer. NOTE: this tracks presses, not Zoom's real
  // state (HID is one-way), so it can drift if Zoom is changed elsewhere.
  if (ledLit[i]){
    ledLit[i]=false;
    ledOff(buttons[i].ledPin);
  }else{
    ledLit[i]=true;
    ledOffAtMs[i]=millis()+OFF_DELAY;
    ledOn(buttons[i].ledPin);
  }

  Serial.print("\nButton pressed on pin ");
  Serial.println(buttons[i].pin);
  sendAltShortcut(buttons[i].key);
}

// Tell the box that Zoom is muted right now. Sends nothing to the host, so it
// can never flip Zoom. Clears the unmuted state and timer, then blinks the LED
// three times as confirmation.
void resyncMuted(uint8_t i){
  ledLit[i]=false;
  Serial.print("\nResync to muted on pin ");
  Serial.println(buttons[i].pin);
  for (uint8_t n=0; n < 3; n++){
    ledOn(buttons[i].ledPin);
    delay(100);
    ledOff(buttons[i].ledPin);
    delay(100);
  }
}

//=========================================
// setup(): one-time initialization
//=========================================
void setup() {
  Serial.begin(9600);   // Debug/serial-monitor output at 9600 baud

  // First pass: light every LED and set each button pin to INPUT_PULLUP.
  // The 100 ms spacing gives a visible "walk" across the LEDs at startup.
  for (uint8_t i=0; i < NUMBER_OF_BUTTONS; i++) {
    if (buttons[i].ledPin!=NO_LED) pinMode(buttons[i].ledPin, OUTPUT);
    ledOn(buttons[i].ledPin);
    pinMode(buttons[i].pin, INPUT_PULLUP);
    delay(100);
  }

  // Second pass: LEDs off, state cleared, resting button state reported
  // over serial (1 = released).
  for (uint8_t i=0; i < NUMBER_OF_BUTTONS; i++) {
    ledOff(buttons[i].ledPin);
    ledLit[i]=false;
    ledOffAtMs[i]=0;
    delay(100);

    bool pressed = (digitalRead(buttons[i].pin)==LOW);
    // A button held at power-up must be released before it can fire.
    stablePressed[i]=pressed;
    holdHandled[i]=true;   // a press already in progress at boot is ignored
    lastReading[i]=pressed;
    lastChangeMs[i]=millis();

    Serial.print(buttons[i].pin);
    Serial.print("=");
    Serial.println(pressed ? 0 : 1);
  }
  Keyboard.begin();   // Begin acting as a USB HID keyboard
}

// loop(): scan buttons without blocking, manage LED timers, send shortcuts.
void loop() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < NUMBER_OF_BUTTONS ; i++)
  {
    // --- LED auto-off (wraparound-safe millis() comparison) ---
    // For Mute, a lit LED means "unmuted": if nobody presses again within
    // OFF_DELAY, send the shortcut once more to put them back on mute.
    if (ledLit[i] && (int32_t)(now - ledOffAtMs[i]) >= 0){
      ledLit[i]=false;
      ledOff(buttons[i].ledPin);
      if (buttons[i].remuteOnTimeout){
        Serial.print("\nAuto re-mute on pin ");
        Serial.println(buttons[i].pin);
        sendAltShortcut(buttons[i].key);
      }
    }

    // --- Debounced edge detection ---
    // INPUT_PULLUP: the pin reads LOW while the button is held.
    bool reading = (digitalRead(buttons[i].pin)==LOW);
    if (reading != lastReading[i]){
      lastReading[i]=reading;
      lastChangeMs[i]=now;
    }
    if (reading != stablePressed[i] && (now - lastChangeMs[i]) >= DEBOUNCE_MS){
      stablePressed[i]=reading;
      if (buttons[i].holdToResync){
        // Resync buttons fire on RELEASE, so a long hold can be told apart
        // from a normal press before any keystroke is sent.
        if (reading){
          pressStartMs[i]=now;
          holdHandled[i]=false;
        }else if (!holdHandled[i]){
          onPress(i);
        }
      }else if (reading){
        onPress(i);   // fire once per press, on the press edge only
      }
    }

    // --- Long hold on a resync button: reset to "muted", send nothing ---
    if (buttons[i].holdToResync && stablePressed[i] && !holdHandled[i] &&
        (now - pressStartMs[i]) >= RESYNC_HOLD_MS){
      holdHandled[i]=true;
      resyncMuted(i);
    }
  }
}
