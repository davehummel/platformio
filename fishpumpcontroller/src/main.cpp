#include "Arduino.h"

#include "SPI.h"
#include "dh_rf24client.h"
#include <EEPROM.h>


#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

RF24Client rfClient(14,10);

#define LEFT_PUMP 4
#define RIGHT_PUMP 3
#define TOPOFF_PUMP 18

byte clientName[5]  = {DEFCLIENTNAME};


void reset(double input){
  Serial.println("Message received!");
  if (input == 1){ // reset Sensor
    Serial.println("Full Restart");
    WRITE_RESTART(0x5FA0004);
    delay(10000);
  }
}


void setup()
{

  analogWriteResolution(10);
  analogWriteFrequency(3, 46875);
  analogWriteFrequency(4, 46875);

  pinMode(RIGHT_PUMP,OUTPUT);
  pinMode(LEFT_PUMP,OUTPUT);
  pinMode(TOPOFF_PUMP,OUTPUT);

  digitalWrite(TOPOFF_PUMP, LOW);
  analogWrite(LEFT_PUMP,512);
  analogWrite(RIGHT_PUMP,512);

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

  rfClient.setListener(25,&reset);


  rfClient.begin();


  delay(1000);

}

void setPWMPump(uint8_t pin, double val){
  if (val > 255){
    val = 255;
  }else if (val < 0 ){
      val = 0;
  }
  uint16_t mappedVal = val*2.588; // make 10 bit but also adjust 12v down to 10v
  analogWrite(pin,mappedVal);
}

void setDigitalPump(uint8_t pin, bool enabled){
  digitalWrite(pin,enabled);
}

elapsedMillis topoffDisableTime = 600000;
uint32_t topoffRunningCount = 0;
uint32_t replyValues = 0;
void loop()
{
  if (replyValues == 10){
    rfClient.sendDouble(0,rfClient.getValue(0));
    delay(1);
    rfClient.sendDouble(1,rfClient.getValue(1));
    delay(1);
    rfClient.sendDouble(2,rfClient.getValue(2));
    replyValues = 0;
  }
  replyValues++;

  rfClient.listen();
  delay(100);

  if(rfClient.getTimeSinceLastPing()>10000){
    Serial.println("Lost contact with controller!");
     setPWMPump(LEFT_PUMP,165);
     setPWMPump(RIGHT_PUMP,165);
     setDigitalPump(TOPOFF_PUMP,false);
  } if(rfClient.getTimeSinceLastPing()>600000){
      Serial.println("Lost contact for 10 minutes , restarting!");
      reset(1);
  } else {
    setPWMPump(LEFT_PUMP,rfClient.getValue(0));
    setPWMPump(RIGHT_PUMP,rfClient.getValue(1));
    if (topoffDisableTime<600000){
      setDigitalPump(TOPOFF_PUMP, false);
    } else {
      if (rfClient.getValue(2)>0){
        setDigitalPump(TOPOFF_PUMP, true);
        topoffRunningCount+=3;
        if (topoffRunningCount>=600){
          topoffRunningCount = 0;
          Serial.println("Disabling topoff for 10 mintues .. running more than 20 sec in last 60");
          topoffDisableTime = 0;
        }
      } else {
        setDigitalPump(TOPOFF_PUMP, false);
        if (topoffRunningCount>0){
          topoffRunningCount--;
        }
      }
    }
  }
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
}
