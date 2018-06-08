#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <TimeLib.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, 21, NEO_GRBW + NEO_KHZ800);
bool isOn = false;

uint8_t hourOn=0,hourOff=12,minOn=0,minOff=0,secOn=0,secOff=0;

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}


void setup() {
  setSyncProvider(getTeensy3Time);
  pinMode(13,OUTPUT);
  digitalWrite(0,LOW);
  pinMode(0,INPUT);

  if (timeStatus()!= timeSet) {
    delay(100000);
  }
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}
uint8_t x = 0;
void loop() {
  if (hour() == hourOn && minute() == minOn && second() == secOn){
    isOn = true;
    delay(1000);
  } else if (hour() == hourOff && minute() == minOff && second() == secOff){
    isOn = false;
    delay(1000);
  }
  if (isOn){
    x=x+1;
    if (x == 60) x = 0;
    strip.setPixelColor(x, 245*isOn,10*isOn, 245*isOn, 150*isOn);
    strip.show();
  }else{
    if (x<60)
      strip.setPixelColor(x, 0, 0, 0,0);
    x = random(256);
    if (x<60)
      strip.setPixelColor(x, 0, 1, 0,0);
    strip.show();
  }
  digitalWrite(13,HIGH);
  delay (200);
  if (digitalRead(0)){
    if (isOn){
      hourOff = hour();
      minOff = minute();
      secOff = second();
    }else{
      hourOn = hour();
      minOn = minute();
      secOn = second();
    }
    isOn = ! isOn;
    x=0;
    while (digitalRead(0))
      delay(10);
    delay(1000);
  }
  digitalWrite(13,LOW);
  delay (200);
}
