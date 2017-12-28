#include "Arduino.h"
#include "i2c_t3.h"

#include "SPI.h"

#include "dh_VL53L0X.h"

#include "dh_rf24client.h"
#include <EEPROM.h>

#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

RF24Client rfClient(15,10);

VL53L0X sensor;

#define SENSOR_EN_PIN 20
#define SENSOR_GND_PIN 17
#define SENSOR_VIN_PIN 16

#define LEVEL_SWITCH_GND 2
#define LEVEL_SWITCH_1 3
#define LEVEL_SWITCH_2 4

byte clientName[5]  = {DEFCLIENTNAME};


void resetSensor(double input){
  Serial.println("Message received!");
  if (input == 1){ // reset Sensor
    Serial.println("Sensor Restart");
    digitalWrite(SENSOR_EN_PIN,LOW);
    delay(100);
    digitalWrite(SENSOR_EN_PIN,HIGH);
    delay(100);
    sensor.setMeasurementTimingBudget(200000); //= 200ms, def is 33
    sensor.init();
    sensor.startContinuous(499);
    for (uint8_t i = 0; i < 6 ; i++){
      digitalWrite(13,i%2);
      delay(150);
    }
    return;
  }
  if (input == 2){ // reset entire device
    Serial.println("Full Restart");
    WRITE_RESTART(0x5FA0004);
    delay(10000);
  }
    digitalWrite(13,HIGH);
    delay(input);
    digitalWrite(13,LOW);
}


void setup()
{
  pinMode(LEVEL_SWITCH_GND,OUTPUT);
  digitalWrite(LEVEL_SWITCH_GND,LOW);

  pinMode(LEVEL_SWITCH_1,INPUT_PULLUP);
  pinMode(LEVEL_SWITCH_2,INPUT_PULLUP);

  pinMode(SENSOR_VIN_PIN,OUTPUT);
  digitalWrite(SENSOR_VIN_PIN,HIGH);
  pinMode(SENSOR_GND_PIN,OUTPUT);
  digitalWrite(SENSOR_GND_PIN,LOW);

  pinMode(SENSOR_EN_PIN,OUTPUT);
  digitalWrite(SENSOR_EN_PIN,HIGH);

  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);



  Wire.setDefaultTimeout(2000);
  Wire.begin();

  SPI.setSCK(14);

  Serial.begin(115200);

  delay(100);

  Serial.println("Starting Radio");

  if (!rfClient.loadClientPrefixFromEEPROM()){
    Serial.println("Failed to load client prefix, using DFLT");
    for (uint8_t i = 0; i < 50 ; i++){
      digitalWrite(13,i%2);
      delay(50);
    }
  }else{
    char prefix[5] = {"0000"};
    rfClient.getClientPrefix(prefix);
    Serial.println(prefix);
  }

  rfClient.setListener(25,&resetSensor);


  rfClient.begin();

  Serial.println("Starting Sensor");

  resetSensor(1);

  digitalWrite(13,HIGH);
  delay(1000);
  digitalWrite(13,LOW);
}

uint8_t failedCount = 0;
bool flipper = false;
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
    } else {
      while (Serial.available()) Serial.read();
    }
  }

  if (countDown>=1000){

    countDown = countDown - 1000;

    digitalWrite(13,flipper);
    flipper = !flipper;

    double distance = sensor.readRangeContinuousMillimeters(false);
    if (distance == 65535){
      //failed once;
      Serial.println("Failed Read");
      for (uint8_t i = 0 ; i<8 ; i++){
        digitalWrite(13,flipper);
        flipper = !flipper;
        delay( 50);
      }

      failedCount++;

      if (failedCount >= 5){
        failedCount = 0;
        resetSensor(1);
      }

    }else{
      // all good!
      Serial.print("Got Distance:");
      Serial.println(distance);
      rfClient.sendDouble(0,distance);
      failedCount = 0;
    }

    delay(1);
    uint8_t floatHigh = digitalRead(LEVEL_SWITCH_1) ;
    Serial.print("Got float 1:");
    Serial.println(floatHigh);
    rfClient.sendDouble(1,floatHigh);

    delay(1);
    floatHigh = digitalRead(LEVEL_SWITCH_2) ;
    Serial.print("Got float 2:");
    Serial.println(floatHigh);
    rfClient.sendDouble(2,floatHigh);
    if(rfClient.getTimeSinceLastPing()>10000){
      Serial.println("Lost contact with controller!");
    } if(rfClient.getTimeSinceLastPing()>600000){
        Serial.println("Lost contact for 10 minutes , restarting!");
        resetSensor(2);
    }
  }
}
