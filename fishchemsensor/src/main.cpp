#include "Arduino.h"
#include "i2c_t3.h"

#include "SPI.h"

#include "dh_atlas_i2c.h"

#include "dh_rf24client.h"
#include <EEPROM.h>

#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

RF24Client rfClient(14,10);

AtlasSensorI2C phSensor(99);
AtlasSensorI2C salinitySensor(100);
AtlasSensorI2C orpSensor(98);
AtlasSensorI2C doSensor(97);

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

  phSensor.setWire(&Wire);
  doSensor.setWire(&Wire);
  orpSensor.setWire(&Wire);
  salinitySensor.setWire(&Wire);


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
uint8_t sensorCount = 0;
void loop()
{
  delay(10);
  rfClient.listen();
  if (Serial.available()){
    char in = Serial.read();
    if (in == '@'){
      uint8_t prefix [4];
      prefix[0] = Serial.read();
      prefix[1] = Serial.read();
      prefix[2] = Serial.read();
      prefix[3] = Serial.read();
      rfClient.saveClientPrefixToEEPROM(prefix);
      Serial.println("Client Prefix saved to eeprom");
      resetSensor(2);
    } else if (in == '#'){
      uint8_t devNum = Serial.read()-'0';
      char input[20] = {0};
      uint8_t i = 0;
      while (i<29){
        input[i] = Serial.read();
        if (input[i] == '\n' || input[i] == '\r'){
          break;
        }
        i++;
      }
      input[i] = '\0';
      Serial.print("Sending command:");
      Serial.print(input);
      Serial.print(" to device ");
      Serial.print(devNum);
      switch(devNum){

        case 0: phSensor.sendCommand(input,input);Serial.println(" = ph"); break;
        case 1: salinitySensor.sendCommand(input, input);Serial.println(" = salinity"); break;
        case 2: orpSensor.sendCommand(input,input);Serial.println(" = orp"); break;
        case 3: doSensor.sendCommand(input, input);Serial.println(" = do"); break;
        default: Serial.println(" = unknown");

      }

      Serial.print("Response:");
      Serial.println(input);

    } else {
      while (Serial.available()) Serial.read();
    }
  }

  if (countDown>=1000){
      Serial.println("Attempting Read");
      countDown = countDown - 1000;

      double in = 0;
      if (sensorCount == 0)
        in = phSensor.readMeasurement();
      else if (sensorCount == 1)
        in = salinitySensor.readMeasurement();
      else if (sensorCount == 2)
        in = orpSensor.readMeasurement();
      else if (sensorCount == 3)
        in = doSensor.readMeasurement();

      if (in == -9999){
        //failed once;
        Serial.println("Failed Read");
          delay( 1000);

        failedCount++;

        if (failedCount >= 10){
          failedCount = 0;
          resetSensor(2);
        }

      }else{
        // all good!
        Serial.print("Got Sensor(");
        Serial.print(sensorCount);
        Serial.print(")=");
        Serial.println(in);
        rfClient.sendDouble(sensorCount,in);
        failedCount = 0;
      }
      sensorCount++;
      if (sensorCount == 4) sensorCount = 0;
  }
}
