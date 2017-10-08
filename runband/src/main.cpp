#include "Arduino.h"
#include "Battery.h"
#include "Mode.h"
#include "Settings.h"
#include "flashS25FL127_spi.h"
#include "wire.h"
#include <FastLED.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SparkFun_I2C_GPS_Arduino_Library.h>
#include <TinyGPS++.h>

#define PWR_PRESS_PIN A5
#define ALT_PRESS_PIN 6
#define SND_EN1 A0
#define SND_EN2 A1
#define SND_DIN 11


#define NEO_PIN 10
#define NUM_LEDS 18

#define CYCLE_INTERVAL 100000
#define MIN_HOLD_INTERVAL 500

I2CGPS myI2CGPS; // Hook object to the library
TinyGPSPlus gps; // Declare gps object
CRGB leds[NUM_LEDS];
Settings settings;
Mode mode;
RTCZero rtc; // Create RTC object
Battery battery;
uint32_t lastInteractionMillis = 0;

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  pinMode(ALT_PRESS_PIN, INPUT_PULLDOWN);

  pinMode(SND_EN1, OUTPUT);
  digitalWrite(SND_EN1, HIGH);
  pinMode(SND_EN2, OUTPUT);
  digitalWrite(SND_EN2, LOW);

  pinMode(PWR_PRESS_PIN, INPUT);

  pinMode(SND_DIN, OUTPUT);

  FastLED.addLeds<NEOPIXEL, NEO_PIN>(leds, NUM_LEDS);

  {
    leds[0] = CRGB::Blue;
    FastLED.show();
    delay(100);
  }

  if (myI2CGPS.begin() == false) {
    uint16_t i = 0;
    while (true) {
      i++;
      if (i > 20)
        digitalWrite(PWR_SUSTAIN_PIN, LOW);
      leds[1] = CRGB::Red;
      FastLED.show();
      delay(300);
      leds[1] = CRGB::Black;
      FastLED.show();
      delay(300);
    }
  }
  leds[1] = CRGB::Blue;
  FastLED.show();

  if (settings.init() == false) {
    uint16_t i = 0;
    while (true) {
      i++;
      if (i > 20)
        digitalWrite(PWR_SUSTAIN_PIN, LOW);
      leds[2] = CRGB::Red;
      FastLED.show();
      delay(300);
      leds[2] = CRGB::Black;
      FastLED.show();
      delay(300);
    }
  }

  settings.loadSettings();
  leds[2] = CRGB::Blue;
  FastLED.show();
  delay(300);
  rtc.begin();
  mode.setResources(&settings,&rtc, &gps, leds, &battery);
  mode.setState(Start_Mode);
}

bool needToTurnOff(void) {
  if (millis() - lastInteractionMillis > 60000*30)
    return true;

 if (battery.getPower()<.1)
    return true;

return false;
}

uint32_t btn1PressedTime = 0;
uint32_t btn2PressedTime = 0;

Button_Event checkButtons(void) {
  bool btn1 = analogRead(PWR_PRESS_PIN) > 1020;
  bool btn2 = digitalRead(ALT_PRESS_PIN);
  digitalWrite(13, btn1);
  uint32_t interactionTime = millis();
  if (btn1 || btn2) {
    lastInteractionMillis = interactionTime;
  }

  if (!btn1 && btn1PressedTime > 0){
    uint32_t pressTime = interactionTime - btn1PressedTime;
    btn1PressedTime = 0;
    if (pressTime>=MIN_HOLD_INTERVAL){
      return Btn1_hold_release;
    }else{
      return Btn1_pressed;
    }
  } else if (btn1) {
    if (btn1PressedTime>0){
      uint32_t pressTime = interactionTime - btn1PressedTime ;
      if (pressTime>MIN_HOLD_INTERVAL){
        return Btn1_hold;
      }
    }else{
      btn1PressedTime = interactionTime;
      return Btn_none;
    }
  }

  if (!btn2 && btn2PressedTime > 0){
    uint32_t pressTime = interactionTime - btn2PressedTime ;
    btn2PressedTime = 0;
    if (pressTime>=MIN_HOLD_INTERVAL){
      return Btn2_hold_release;
    }else{
      return Btn2_pressed;
    }
  } else if (btn2) {
    if (btn2PressedTime>0){
      uint32_t pressTime = interactionTime- btn2PressedTime;
      if (pressTime>MIN_HOLD_INTERVAL){
        return Btn2_hold;
      }
    }else{
      btn2PressedTime = interactionTime;
      return Btn_none;
    }
  }

  return Btn_none;
}

void loop() {
  uint32_t currentTime = 0;
  uint32_t lastTime = 0;
  bool rtcIsValid = false;
  while (!needToTurnOff()) {
    while (myI2CGPS.available()) // available() returns the number of new bytes
    {
      gps.encode(myI2CGPS.read()); // Feed the GPS parser
    }

    if (!rtcIsValid) {
      if (gps.time.isValid()) {
        rtcIsValid = true;
        rtc.setHours(gps.time.minute());
        rtc.setMinutes(gps.time.minute());
        rtc.setSeconds(gps.time.second());

        // Set the date
        rtc.setDay(gps.date.day());
        rtc.setMonth(gps.date.month());
        rtc.setYear(gps.date.year()-2000);
      }
    } else {
      Serial.print(F("Date: "));
      Serial.print(rtc.getMonth());
      Serial.print(F("/"));
      Serial.print(rtc.getDay());
      Serial.print(F("/"));
      Serial.print(rtc.getYear()+2000);

      Serial.print((" Time: "));

      Serial.print(rtc.getHours());
      Serial.print(F(":"));

      Serial.print(rtc.getMinutes());
      Serial.print(F(":"));

      Serial.print(rtc.getSeconds());
      Serial.println(); // Done printing time
    }
    Button_Event event = checkButtons();
    if (event != Btn_none) {
      mode.input(event);
    }
    mode.render();
    currentTime = micros();
    uint32_t dif = currentTime - lastTime;
    if (dif < CYCLE_INTERVAL) {
      delayMicroseconds(CYCLE_INTERVAL - dif);
      lastTime += CYCLE_INTERVAL;
    } else {
      lastTime = currentTime;
    }
  }

  battery.off();

}
