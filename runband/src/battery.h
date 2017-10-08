#ifndef DH_BATTERY_H__
#define DH_BATTERY_H__

#include "Arduino.h"

#define BAT_VOLTAGE_PIN A7
#define PWR_SUSTAIN_PIN 12

#define MIN_VOLTAGE 3.2
#define MAX_VOLTAGE 4.15

class Battery {
public:
  Battery() { pinMode(BAT_VOLTAGE_PIN, INPUT);  pinMode(PWR_SUSTAIN_PIN, OUTPUT);
    digitalWrite(PWR_SUSTAIN_PIN, HIGH);
   }
  float getPower(void) {
    float measuredvbat = analogRead(BAT_VOLTAGE_PIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    if (measuredvbat <= MIN_VOLTAGE) {
      return 0.0;
    }
    if (measuredvbat >= MAX_VOLTAGE) {
      return 1.0;
    }
    return ((measuredvbat - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE));
  }

  void off(void){
    digitalWrite(PWR_SUSTAIN_PIN,LOW);
    while (true) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
  }

};

#endif
