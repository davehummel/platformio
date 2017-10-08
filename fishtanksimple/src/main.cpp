#include "Arduino.h"
#include "i2c_t3.h"
#include "dh_logger.h"
#include "dh_controller.h"
#include "OneWire.h"
#include "ControlledOSStatus.h"

#include "ControlledCalc.h"
#include "ControlledLED.h"
// #include "ControlledThermometer.h"


Logger logger;

ControlledOSStatus os;

ControlledCalc calc;

ControlledLED led;

//ControlledADC adc;
// uint8_t depthMeasureEnablePins[2] = {17,16};
// ControlledVL53L0X waterDepth(2,depthMeasureEnablePins);

// byte thermoIDs[3] = { 0xAD , 0xC5 , 0x48};
// ControlledThermometer thermo(24,thermoIDs, 3);

Controller controller;


void setup() {

    delay(100);

  Serial.begin(115200);
  Serial1.begin(115200);

    delay(1000);

    while (Serial.available() == 0){

    }
    Serial.flush();


  controller.setOutputStream(&Serial1);
  controller.setErrorStream(&Serial);


Serial.println("Starting Controlled Modules");

  // controller.loadControlled('A',&adc);
  //
   controller.loadControlled('P',&led);

  controller.loadControlled('Z',&os);

  controller.loadControlled('C',&calc);

  // controller.loadControlled('T',&thermo);

//    controller.loadControlled('W',&waterDepth);

  Serial.println("Modules have started!!");


controller.run(2,Controller::newString("Enable"),'T');

    controller.run(1,Controller::newString("B ZZZ 1 0"),'Z',1);

    controller.run(2,Controller::newString("BLK Z 13 1000"),'P');

      controller.schedule(1,2000,0,false,1,Controller::newString("B DDD 1 0"),'P',1);

}

void loop() {
        controller.loop(&Serial);
}
