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
 *   - LEDs are optional. A button whose LED constant is NO_LED simply skips
 *     all LED handling, so the box works with or without LEDs installed.
 * ------------------------------------------------------------------------
 */

#include <HID-Project.h>   // Provides BootKeyboard for native USB HID output


// Number of physical buttons wired to the box. This sizes the buttons[],
// leds[] and ledsOnUntil[] arrays and bounds the main scan loop.
#define NUMBER_OF_BUTTONS 4


//real settings
// How long (in milliseconds) a button's LED stays lit after a press before
// it is automatically turned off. 180000 ms = 3 minutes.
#define OFF_DELAY 180000 //3 minutes

// --- Button-to-pin mapping ------------------------------------------------
// Each BTN_* constant is the digital pin number the corresponding button is
// wired to. These pin numbers double as human-readable identifiers in the
// switch() below, which is why the switch keys on the pin rather than the
// loop index.
//
// NOTE: several constants below (GALLERVIEW/SPEAKERVIEW/MUTEALL) are all set
// to pin 10 as placeholders for features that are defined but not yet wired
// into the active buttons[] list or the switch() dispatch.
#define BTN_MUTE    1
#define BTN_RAISEHAND  2
#define BTN_VIDEOTOGGLE 6
#define BTN_FULLSCREEN  8
#define BTN_GALLERVIEW  10
#define BTN_SPEAKERVIEW 10
#define BTN_MUTEALL   10
//#define BTN_SHARE   10



// Sentinel pin value meaning "this button has no LED". ledOn()/ledOff()
// ignore any pin equal to NO_LED, so LED wiring is entirely optional.
#define NO_LED 99

// --- LED-to-pin mapping ---------------------------------------------------
// Digital pin driving each button's indicator LED. All are currently NO_LED,
// so no LEDs are driven; set these to real output pins to enable feedback.
#define LED_MUTE    NO_LED
#define LED_RAISEHAND NO_LED
#define LED_VIDEOTOGGLE NO_LED
#define LED_FULLSCREEN  NO_LED
#define LED_GALLERVIEW  NO_LED
#define LED_SPEAKERVIEW NO_LED
#define LED_MUTEALL   NO_LED
#define LED_SHARE   NO_LED



// Active buttons scanned by the main loop, in scan order.
int buttons[NUMBER_OF_BUTTONS] = {BTN_RAISEHAND, BTN_VIDEOTOGGLE, BTN_FULLSCREEN, BTN_MUTE};
// LED pin paired with each button, index-aligned to buttons[].
// (Harmless while every entry is NO_LED, but note the ordering does not
//  match buttons[]: index 2 is BTN_FULLSCREEN yet uses LED_MUTE, and index 3
//  is BTN_MUTE yet uses LED_FULLSCREEN. Re-align these if real LEDs are added.)
int leds[NUMBER_OF_BUTTONS] = {LED_RAISEHAND, LED_VIDEOTOGGLE, LED_MUTE, LED_FULLSCREEN};

// Per-button timestamp (millis) at which the LED should switch off.
// A value of 0 means "LED is off / no timer running".
uint32_t ledsOnUntil[NUMBER_OF_BUTTONS];


//=========================================
// LED helpers
//
// Both helpers no-op when passed NO_LED, which keeps the rest of the code
// free of "does this button have an LED?" checks.
//=========================================
// Turn an LED on, unless the pin is the NO_LED sentinel.
void ledOn(int parPin){
  if (parPin!=NO_LED)
  {
    digitalWrite(parPin, HIGH);
  }
}
// Turn an LED off, unless the pin is the NO_LED sentinel.
void ledOff(int parPin){
  if (parPin!=NO_LED)
  {
    digitalWrite(parPin, LOW);
  }
}
//=========================================
// setup(): one-time initialization
//
// Runs a brief LED self-test, configures the button pins as pull-up inputs,
// prints the initial button states over serial for debugging, and starts
// the USB HID boot keyboard.
//=========================================
void setup() {
  Serial.begin(9600);   // Debug/serial-monitor output at 9600 baud
  //  Serial.write("Starting...\n");
  //  Serial.end();

  // First pass: light every LED and set each button pin to INPUT_PULLUP.
  // The 100 ms spacing gives a visible "walk" across the LEDs at startup.
  for (int i=0; i < NUMBER_OF_BUTTONS; i++) {
    ledOn(leds[i]);
    pinMode(buttons[i], INPUT_PULLUP);
    delay(100);
  }

  // Second pass: turn the LEDs back off, clear each auto-off timer, and
  // report the resting state of every button over serial (1 = released).
  for (int i=0; i < NUMBER_OF_BUTTONS; i++) {
    ledOff(leds[i]);
    ledsOnUntil[i]=0;
    delay(100);

    Serial.print(buttons[i]);
    Serial.print("=");
    Serial.println(digitalRead(buttons[i]));
  }
  BootKeyboard.begin();   // Begin acting as a USB HID boot keyboard
  //delay(100);
  //Serial.println(__TIME__);
}

// loop(): continuously scan buttons, manage LED timers, and send shortcuts.
void loop() {

  /*
  //---code to check buttons state (for debug only)
  static uint32_t nexRepTime=0;
  if (millis()>nexRepTime)
  {
    Serial.print(millis());
    for (int i = 0; i < NUMBER_OF_BUTTONS ; i++)
    {
      Serial.print(buttons[i]);
      Serial.print("=");
      Serial.print(digitalRead(buttons[i]));
      Serial.print(";\t");
    }
    Serial.println("---------------");
    nexRepTime=millis()+500;
  }
  */
  for (int i = 0; i < NUMBER_OF_BUTTONS ; i++)
  {
    // --- LED auto-off ---
    // If a timer is running for this button and its deadline has passed,
    // turn the LED off and clear the timer.
    if (ledsOnUntil[i]!=0){
      if (ledsOnUntil[i]<millis()){
        ledOff(leds[i]);
        ledsOnUntil[i]=0;
      }
    }
    // --- Button press detection ---
    // A pin reads LOW while its button is held (pull-up + button-to-ground).
    if (digitalRead(buttons[i])==LOW){
      // Toggle the LED timer: a press while the LED is already on turns it
      // off immediately; otherwise light the LED and arm the OFF_DELAY timer.
      if (ledsOnUntil[i]!=0){
        ledsOnUntil[i]=0;

        ledOff(leds[i]);
      }else{
        ledsOnUntil[i]=millis()+OFF_DELAY;
        ledOn(leds[i]);
      }

      Serial.print("\nButton pressed on pin ");
      Serial.println(buttons[i]);
      // Dispatch on the button's pin number (not the loop index) so each
      // case reads as the feature it maps to. Each case types an Alt+<key>
      // Zoom shortcut: hold Alt, tap the letter, then release Alt. The small
      // delays give the host time to register the modifier and key.
      switch (buttons[i]){ //here we use button pin instead of i - to make code more readable and flexible
        case BTN_RAISEHAND:{        // Alt + Y : raise / lower hand
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_Y);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        case BTN_MUTE:{             // Alt + A : mute / unmute audio
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_A);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        case BTN_VIDEOTOGGLE:{      // Alt + V : start / stop video
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_V);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        case BTN_FULLSCREEN:{       // Alt + F : enter / exit full screen
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_F);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        default:                    // Pin with no mapped shortcut
        Serial.println("Unknown case:");
        Serial.print(i);
        break;
      } //\switch (buttons[i])...
      // Debounce / block until the button is released so a single press
      // sends exactly one shortcut. Poll every 10 ms until the pin is HIGH.
      //debounce and wait while button is released
      //delay(50);
      while(digitalRead(buttons[i])==LOW){
        delay(10);
      }

    }//\if (digitalRead(buttons[i])==LOW)...
  } //\for (int i = 0; i < NUMBER_OF_BUTTONS ; i++)...

}
