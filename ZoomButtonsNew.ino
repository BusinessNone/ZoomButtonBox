#include <HID-Project.h>


#define NUMBER_OF_BUTTONS 4


//real settings
#define OFF_DELAY 180000 //3 minutes

#define BTN_MUTE    1
#define BTN_RAISEHAND  2
#define BTN_VIDEOTOGGLE 6
#define BTN_FULLSCREEN  8
#define BTN_GALLERVIEW  10
#define BTN_SPEAKERVIEW 10
#define BTN_MUTEALL   10
//#define BTN_SHARE   10



#define NO_LED 99

#define LED_MUTE    NO_LED
#define LED_RAISEHAND NO_LED
#define LED_VIDEOTOGGLE NO_LED
#define LED_FULLSCREEN  NO_LED
#define LED_GALLERVIEW  NO_LED
#define LED_SPEAKERVIEW NO_LED
#define LED_MUTEALL   NO_LED
#define LED_SHARE   NO_LED



int buttons[NUMBER_OF_BUTTONS] = {BTN_RAISEHAND, BTN_VIDEOTOGGLE, BTN_FULLSCREEN, BTN_MUTE};
int leds[NUMBER_OF_BUTTONS] = {LED_RAISEHAND, LED_VIDEOTOGGLE, LED_MUTE, LED_FULLSCREEN};

uint32_t ledsOnUntil[NUMBER_OF_BUTTONS];


//=========================================
//
//=========================================
void ledOn(int parPin){
  if (parPin!=NO_LED)
  {
    digitalWrite(parPin, HIGH);
  }
}
void ledOff(int parPin){
  if (parPin!=NO_LED)
  {
    digitalWrite(parPin, LOW);
  }
}
//=========================================
//
//=========================================
void setup() {
  Serial.begin(9600);
  //  Serial.write("Starting...\n");
  //  Serial.end();
  for (int i=0; i < NUMBER_OF_BUTTONS; i++) {
    ledOn(leds[i]);
    pinMode(buttons[i], INPUT_PULLUP);
    delay(100);
  }

  for (int i=0; i < NUMBER_OF_BUTTONS; i++) {
    ledOff(leds[i]);
    ledsOnUntil[i]=0;
    delay(100);
    
    Serial.print(buttons[i]);
    Serial.print("=");
    Serial.println(digitalRead(buttons[i]));
  }
  BootKeyboard.begin();
  //delay(100);
  //Serial.println(__TIME__);
}

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
    if (ledsOnUntil[i]!=0){
      if (ledsOnUntil[i]<millis()){
        ledOff(leds[i]);
        ledsOnUntil[i]=0;
      }
    }
    if (digitalRead(buttons[i])==LOW){
      if (ledsOnUntil[i]!=0){
        ledsOnUntil[i]=0;
        
        ledOff(leds[i]);
      }else{
        ledsOnUntil[i]=millis()+OFF_DELAY;
        ledOn(leds[i]);
      }
      
      Serial.print("\nButton pressed on pin ");
      Serial.println(buttons[i]);
      switch (buttons[i]){ //here we use button pin instead of i - to make code more readable and flexible
        case BTN_RAISEHAND:{
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_Y);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        case BTN_MUTE:{
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_A);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        case BTN_VIDEOTOGGLE:{
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_V);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        case BTN_FULLSCREEN:{
          BootKeyboard.press(KEY_LEFT_ALT);
          delay(50);
          BootKeyboard.write(KEY_F);
          delay(100);
          BootKeyboard.release(KEY_LEFT_ALT);
        };
        break;
        default:
        Serial.println("Unknown case:");
        Serial.print(i);
        break;
      } //\switch (buttons[i])...
      //debounce and wait while button is released
      //delay(50);
      while(digitalRead(buttons[i])==LOW){
        delay(10);
      }
      
    }//\if (digitalRead(buttons[i])==LOW)...
  } //\for (int i = 0; i < NUMBER_OF_BUTTONS ; i++)...
  
}
