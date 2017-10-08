#include "Arduino.h"
#include "i2c_t3.h"
#include "dh_logger.h"
#include "dh_controller.h"
#include "OneWire.h"
//#include "SPI.h"

//#include "ILI9341_t3.h"
//#include <XPT2046_Touchscreen.h>
#include <dh_VL53L0X.h>
//#include "CTFont.h"

#include "ControlledOSStatus.h"
//#include "ControlledTouchLCD.h"
#include "ControlledCalc.h"
#include "ControlledLED.h"
//#include "ControlledADC.h"
#include "ControlledThermometer.h"
#include "ControlledVL53L0X.h"


Logger logger;

ControlledOSStatus os;

ControlledCalc calc;

ControlledLED led;

//ControlledTouchLCD disp;

//ControlledADC adc;
uint8_t depthMeasureEnablePins[3] = {17,16,15};
ControlledVL53L0X waterDepth(3,depthMeasureEnablePins);

byte thermoIDs[3] = { 0xAD , 0xC5, 0x48 };
ControlledThermometer thermo(2/*pin 2*/,thermoIDs, 3/* 2 devices*/);

Controller controller;


void setup() {

  // disp.tch_cs = 25;
  // disp.tft_dc = 9;
  // disp.tft_cs = 10;
  // disp.tft_rst = 26;
  // disp.tft_sclk = 27;

  Wire.setDefaultTimeout(2000);
  Wire.begin();

  waterDepth.setWire(&Wire);

    delay(100);

  Serial.begin(1000000);
    Serial1.begin(1000000);

    delay(1000);


  controller.setOutputStream(&Serial1);
  controller.setErrorStream(&Serial);


Serial.println("Starting Controlled Modules");

//  controller.loadControlled('A',&adc);

  controller.loadControlled('P',&led);

  controller.loadControlled('Z',&os);

  controller.loadControlled('C',&calc);

    // controller.loadControlled('D',&disp);

      controller.loadControlled('T',&thermo);

    controller.loadControlled('W',&waterDepth);

  Serial.println("Modules have started!!");

 // controller.run(2,Controller::newString("B BRT 255"),'D',2);
 //
 // controller.run(2,Controller::newString("B ROT 2"),'D',2);

controller.run(2,Controller::newString("Enable"),'T');
controller.run(2,Controller::newString("Enable"),'W');

// controller.schedule(10,1000,20,false,0,Controller::newString("D"),'D');

  // Signal startup is complete to listening computer
  controller.schedule(1,2000,0,false,1,Controller::newString("B ZZZ 1 0"),'Z',1);

controller.run(2,Controller::newString("BLK Z 13 1000"),'P');

controller.schedule(5,1000,2000,false,0,Controller::newString("B ZZZ 1"),'P',2);


}

void loop() {
        controller.loop(&Serial1);
}
