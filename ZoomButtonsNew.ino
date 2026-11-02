/*
 * ZoomButtonsNew.ino
 * ------------------------------------------------------------------------
 * Firmware for a small physical "Zoom button box" built on an Arduino board
 * that exposes native USB HID (e.g. Leonardo, Micro, or Pro Micro with an
 * ATmega32U4). Each momentary push button on the box triggers a Zoom
 * keyboard shortcut, so a meeting can be controlled without touching the
 * on-screen controls.
 *
 * How it works
 *   - The board enumerates on the host computer as a USB "boot" keyboard
 *     via the HID-Project library (BootKeyboard). Boot keyboards are
 *     recognized very early by the host, which makes the box work reliably
 *     even at BIOS/login screens.
 *   - Buttons are wired between a digital pin and ground and read with the
 *     internal pull-up resistor (INPUT_PULLUP): the pin reads HIGH when the
 *     button is open and LOW when pressed.
 *   - When a button is pressed the matching Zoom shortcut (Alt + a letter)
 *     is typed onto the host. An optional per-button LED can be lit to give
 *     visual feedback; the LED auto-extinguishes after OFF_DELAY.
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

#include <HID-Project.h>   // Provides BootKeyboard for native USB HID output

// How long (in milliseconds) a button's LED stays lit after a press before
// it is automatically turned off. 180000 ms = 3 minutes.
#define OFF_DELAY 180000UL

// A press/release is accepted only after the pin has been stable this long.
#define DEBOUNCE_MS 30

// Sentinel pin value meaning "this button has no LED". ledOn()/ledOff()
// ignore any pin equal to NO_LED, so LED wiring is entirely optional.
#define NO_LED 99

// --- Pin mapping ----------------------------------------------------------
#define BTN_MUTE        1
#define BTN_RAISEHAND   2
#define BTN_VIDEOTOGGLE 6
#define BTN_FULLSCREEN  8
// Not implemented yet (left commented out; they all shared placeholder pin 10):
//#define BTN_GALLERVIEW  10
//#define BTN_SPEAKERVIEW 10
//#define BTN_MUTEALL     10
//#define BTN_SHARE       10

// Digital pin driving each button's indicator LED. All NO_LED by default;
// set to real output pins to enable feedback.
#define LED_MUTE        NO_LED
#define LED_RAISEHAND   NO_LED
#define LED_VIDEOTOGGLE NO_LED
#define LED_FULLSCREEN  NO_LED

// One row per button: its pin, its LED pin, and the key sent with Alt.
// Keeping the three together means a button, its LED and its shortcut can
// never get out of step with each other.
struct Button {
  uint8_t pin;
  uint8_t ledPin;
  KeyboardKeycode key;
};

const Button buttons[] = {
  { BTN_RAISEHAND,   LED_RAISEHAND,   KEY_Y },  // Alt + Y : raise / lower hand
  { BTN_VIDEOTOGGLE, LED_VIDEOTOGGLE, KEY_V },  // Alt + V : start / stop video
  { BTN_FULLSCREEN,  LED_FULLSCREEN,  KEY_F },  // Alt + F : enter / exit full screen
  { BTN_MUTE,        LED_MUTE,        KEY_A },  // Alt + A : mute / unmute audio
};
const uint8_t NUMBER_OF_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

// Per-button runtime state (parallel to buttons[]).
bool     stablePressed[NUMBER_OF_BUTTONS];   // debounced state: true = held
bool     lastReading[NUMBER_OF_BUTTONS];     // last raw reading
uint32_t lastChangeMs[NUMBER_OF_BUTTONS];    // when the raw reading last changed
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
void sendAltShortcut(KeyboardKeycode parKey){
  BootKeyboard.press(KEY_LEFT_ALT);
  delay(50);
  BootKeyboard.write(parKey);
  delay(100);
  BootKeyboard.release(KEY_LEFT_ALT);
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
    lastReading[i]=pressed;
    lastChangeMs[i]=millis();

    Serial.print(buttons[i].pin);
    Serial.print("=");
    Serial.println(pressed ? 0 : 1);
  }
  BootKeyboard.begin();   // Begin acting as a USB HID boot keyboard
}

// loop(): scan buttons without blocking, manage LED timers, send shortcuts.
void loop() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < NUMBER_OF_BUTTONS ; i++)
  {
    // --- LED auto-off (wraparound-safe millis() comparison) ---
    if (ledLit[i] && (int32_t)(now - ledOffAtMs[i]) >= 0){
      ledLit[i]=false;
      ledOff(buttons[i].ledPin);
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
      if (reading) onPress(i);   // fire once per press, on the press edge only
    }
  }
}
