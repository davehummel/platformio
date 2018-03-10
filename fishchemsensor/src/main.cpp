#include "Arduino.h"
#include "i2c_t3.h"

#include "SPI.h"

#include "dh_atlas_ph_i2c.h"

#include "dh_rf24client.h"
#include <EEPROM.h>

#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

RF24Client rfClient(14,10);
PHSensorI2C sensor;

byte clientName[5]  = {DEFCLIENTNAME};


void resetSensor(double input){
  if (input == 1){ // reset Sensor

    return;
  }
  if (input == 2){ // reset entire device
    Serial.println("Full Restart");
    WRITE_RESTART(0x5FA0004);
    delay(10000);
  }

}


void setup()
{


  Wire.setDefaultTimeout(2000);
  Wire.begin();

  sensor.setWire(&Wire);

  SPI.setSCK(13);

  Serial.begin(115200);


  delay(5000);

  Serial.println("Starting Radio");


  if (!rfClient.loadClientPrefixFromEEPROM()){
    Serial.println("Failed to load client prefix, using DFLT");
      delay(50);
  }else{
    char prefix[5] = {"0000"};
    rfClient.getClientPrefix(prefix);
    Serial.println(prefix);
  }

  rfClient.setListener(25,&resetSensor);


  rfClient.begin();

  Serial.println("Starting Sensor");

  resetSensor(1);

  delay(1000);

}

uint8_t failedCount = 0;
elapsedMillis countDown = 0;
void loop()
{
  delay(10);
  rfClient.listen();
  if (Serial.available()>=5){
    if (Serial.read() == '@'){
      uint8_t prefix [4];
      prefix[0] = Serial.read();
      prefix[1] = Serial.read();
      prefix[2] = Serial.read();
      prefix[3] = Serial.read();
      rfClient.saveClientPrefixToEEPROM(prefix);
      Serial.println("Client Prefix saved to eeprom");
      resetSensor(2);
    } else {
      while (Serial.available()) Serial.read();
    }
  }

  if (countDown>=1000){
      Serial.println("Attempting Read");
    countDown = countDown - 1000;

    double ph = sensor.readMeasurement();
    if (ph == -1){
      //failed once;
      Serial.println("Failed Read");
        delay( 500);

      failedCount++;

      if (failedCount >= 5){
        failedCount = 0;
        resetSensor(1);
      }

    }else{
      // all good!
      Serial.print("Got PH:");
      Serial.println(ph);
      rfClient.sendDouble(0,ph);
      failedCount = 0;
    }

  }
}
