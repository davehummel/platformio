#include "Arduino.h"

#include "SPI.h"
#include "dh_rf24client.h"
#include <EEPROM.h>

#define RESTART_ADDR 0xE000ED0C
#define READ_RESTART() (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

RF24Client rfClient(14, 10);

#define LEFT_PUMP 4
#define RIGHT_PUMP 3
#define TOPOFF_PUMP 18

#define DEF_POWER_LEFT 85
#define DEF_POWER_RIGHT 90

byte clientName[5] = {DEFCLIENTNAME};

double leftVal, rightVal;
bool topoffVal;

void reset(double input)
{
  Serial.println("Message received!");
  if (input == 1)
  { // reset Sensor
    Serial.println("Full Restart");
    WRITE_RESTART(0x5FA0004);
    delay(10000);
  }
}

void setup()
{

  analogWriteResolution(8);
  analogWriteFrequency(3, 46875);
  analogWriteFrequency(4, 46875);

  pinMode(RIGHT_PUMP, OUTPUT);
  pinMode(LEFT_PUMP, OUTPUT);
  pinMode(TOPOFF_PUMP, OUTPUT);

// Initialize with topoff pump off and left and right at a very low power (but not off)
  digitalWrite(TOPOFF_PUMP, LOW);
  analogWrite(LEFT_PUMP, DEF_POWER_LEFT);
  analogWrite(RIGHT_PUMP, DEF_POWER_RIGHT);

    leftVal = DEF_POWER_LEFT;
    rfClient.setValue(0, DEF_POWER_LEFT);
    rightVal = DEF_POWER_RIGHT;
    rfClient.setValue(1, DEF_POWER_RIGHT);
  topoffVal = false;
      rfClient.setValue(2, 0);

  Serial.begin(115200);

  delay(100);

  Serial.println("Starting Radio");

  if (!rfClient.loadClientPrefixFromEEPROM())
  {
    Serial.println("Failed to load client prefix, using DFLT");
    for (uint8_t i = 0; i < 50; i++)
    {
      digitalWrite(13, i % 2);
      delay(50);
    }
  }
  else
  {
    char prefix[5] = {"0000"};
    rfClient.getClientPrefix(prefix);
    Serial.println(prefix);
  }

  rfClient.setListener(25, &reset);

  rfClient.begin();

  delay(1000);
}

void setPWMPump(uint8_t pin, double val)
{
  if (val > 255)
  {
    val = 255;
  }
  else if (val < 0)
  {
    val = 0;
  }

  analogWrite(pin, val);
}

void setDigitalPump(uint8_t pin, bool enabled)
{
  digitalWrite(pin, enabled);
}

elapsedMillis topoffDisableTime = 600000;
elapsedMillis millisSinceTopoffFalse = 0;
uint32_t topoffRunningCount = 0;
uint32_t replyValues = 0;
void loop()
{
  if (replyValues == 10)
  {
    rfClient.sendDouble(0, leftVal);
    delay(1);
    rfClient.sendDouble(1, rightVal);
    delay(1);
    rfClient.sendDouble(2, (topoffVal ? 1 : 0));
    replyValues = 0;
  }
  replyValues++;

  rfClient.listen();
  delay(100);

  if (topoffVal == true && millisSinceTopoffFalse > 4000)
  {
    topoffVal = false;
    rfClient.setValue(2, 0);
  }
  if (topoffVal == false)
  {
    millisSinceTopoffFalse = 0;
  }

  if (rfClient.getTimeSinceLastPing() > 600000)
  {
    Serial.println("Lost contact for 10 minutes , restarting!");
    reset(1);
  }
  if (rfClient.getTimeSinceLastPing() > 10000)
  {
    Serial.println("Lost contact with controller!");
    leftVal = DEF_POWER_LEFT;
    rfClient.setValue(0, DEF_POWER_LEFT);
    rightVal = DEF_POWER_RIGHT;
    rfClient.setValue(1, DEF_POWER_RIGHT);
    topoffVal = false;
    rfClient.setValue(2, 0);
  }
  else
  {
    Serial.print(".");
    leftVal = rfClient.getValue(0);
    rightVal = rfClient.getValue(1);
    topoffVal = rfClient.getValue(2) == 1; // ignore all other values
    if (topoffDisableTime < 600000)
    {
      topoffVal = false;
      rfClient.setValue(2, 0);
    }
    else
    {
      if (topoffVal == true)
      {
        topoffRunningCount += 3;
        if (topoffRunningCount >= 600)
        {
          topoffRunningCount = 0;
          Serial.println("Disabling topoff for 10 mintues .. running more than 20 sec in last 60");
          topoffDisableTime = 0;
        }
      }
      else
      {
        if (topoffRunningCount > 0)
        {
          topoffRunningCount--;
        }
      }
    }
  }

  setPWMPump(LEFT_PUMP, leftVal);
  setPWMPump(RIGHT_PUMP, rightVal);
  setDigitalPump(TOPOFF_PUMP, topoffVal);

  Serial.print("Setting Left:");
  Serial.print(leftVal);
  Serial.print(" Right:");
  Serial.print(rightVal);
  Serial.print(" Topoff:");
  Serial.println(topoffVal);

  if (Serial.available() >= 5)
  {
    if (Serial.read() == '@')
    {
      uint8_t prefix[4];
      prefix[0] = Serial.read();
      prefix[1] = Serial.read();
      prefix[2] = Serial.read();
      prefix[3] = Serial.read();
      rfClient.saveClientPrefixToEEPROM(prefix);
      Serial.println("Client Prefix saved to eeprom");
    }
    else
    {
      while (Serial.available())
        Serial.read();
    }
  }
}
