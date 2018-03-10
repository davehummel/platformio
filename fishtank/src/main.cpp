#include "Arduino.h"
// #include "i2c_t3.h"
#include "dh_logger.h"
#include "dh_controller.h"
#include "OneWire.h"
#include <SPI.h>
#include "RF24.h"

//#include "ILI9341_t3.h"
//#include <XPT2046_Touchscreen.h>
//#include "CTFont.h"

#include "ControlledOSStatus.h"
//#include "ControlledTouchLCD.h"
#include "ControlledCalc.h"
#include "ControlledLED.h"
//#include "ControlledADC.h"
#include "ControlledThermometer.h"
#include "ControlledRF24.h"


Logger logger;

ControlledOSStatus os;

ControlledCalc calc;

ControlledLED led;

ControlledRF24 rfClient(25,10);

byte thermoIDs[3] = { 0xAD , 0xC5, 0x48 };
ControlledThermometer thermo(2/*pin 2*/,thermoIDs, 3/* 2 devices*/);

Controller controller;


void setup() {

  pinMode(13, OUTPUT);

 SPI.setSCK(27);

  Serial.begin(1000000);
  Serial1.begin(1000000);

    delay(1000);


    controller.setOutputStream(&Serial1);
    controller.setErrorStream(&Serial);



Serial.println("Starting Controlled Modules");

  controller.loadControlled('P',&led);

  controller.loadControlled('Z',&os);

  controller.loadControlled('C',&calc);

  controller.loadControlled('T',&thermo);

  controller.loadControlled('R',&rfClient);

  Serial.println("Modules have started!!");


controller.run(2,Controller::newString("Enable"),'T');

// Connect to the 4 radio devices
controller.run(2,Controller::newString("CONNECT A 1NOD"),'R');
controller.run(2,Controller::newString("CONNECT B 2NOD"),'R');
controller.run(2,Controller::newString("CONNECT C 3NOD"),'R');
controller.run(2,Controller::newString("CONNECT D 4NOD"),'R');

  // Signal startup is complete to listening computer
  controller.schedule(1,2000,0,false,1,Controller::newString("B ZZZ 1 0"),'Z',1);

controller.run(2,Controller::newString("BLK Z 13 1000"),'P');

controller.schedule(5,1000,2000,false,0,Controller::newString("B ZZZ 1"),'P',2);

controller.run(2,Controller::newString("D CAA 150"),'R',2);
controller.run(2,Controller::newString("D CBB 150"),'R',2);

}

void loop() {
        controller.loop(&Serial1);
}
