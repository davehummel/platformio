#include "Arduino.h"
#include "i2c_t3.h"

#include "SPI.h"

#include "dh_atlas_i2c.h"

#include "dh_rf24client.h"
#include <EEPROM.h>

#include <BME280.h>
#include <CCS811.h>
#include <SCD30.h>

#define RESTART_ADDR 0xE000ED0C
#define READ_RESTART() (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
#define VOC_EN_PIN 17

RF24Client rfClient(14, 10);

BME280 pressureSensor;
CCS811 airSensor(0x5A);
SCD30 co2Sensor;

AtlasSensorI2C phSensor(99);
AtlasSensorI2C salinitySensor(100);
AtlasSensorI2C orpSensor(98);
AtlasSensorI2C doSensor(101);

byte clientName[5] = {DEFCLIENTNAME};

void resetSensor(double input)
{
  if (input == 1)
  { // reset Sensor
    Serial.print("Starting SCD30:");
    CCS811Core::status returnCode = airSensor.begin();
    if (returnCode != CCS811Core::SENSOR_SUCCESS)
    {
      Serial.print(" Problem with CCS811:");
      Serial.println(returnCode);
      digitalWrite(13, HIGH);
    }
    else
    {
      Serial.println("CCS811 online");
    }
    Serial.print("Starting SCD30:");
    bool co2OK = co2Sensor.begin();
    Serial.println(co2OK);
    if (co2OK)
      co2Sensor.setMeasurementInterval(5);

    delay(3000);
    Serial.print("Starting BME280... result of .begin(): 0x");
    Serial.println(pressureSensor.begin(), HEX);
    digitalWrite(VOC_EN_PIN, LOW);
    delay(2000);
    Serial.print("Pressure reading [mbar]=");

    int BMEmBar = pressureSensor.readFloatPressure() / 100;
    Serial.println(BMEmBar);

    //co2Sensor.setAmbientPressure(BMEmBar);
    return;
  }
  if (input == 2)
  { // reset entire device
    Serial.println("Full Restart");
    WRITE_RESTART(0x5FA0004);
    delay(10000);
  }
}

void setup()
{
  pinMode(VOC_EN_PIN, OUTPUT);
  digitalWrite(VOC_EN_PIN, LOW);
  Wire.setDefaultTimeout(2000);
  Wire.begin();
  Wire.setClock(100000);

  phSensor.setWire(&Wire);
  doSensor.setWire(&Wire);
  orpSensor.setWire(&Wire);
  salinitySensor.setWire(&Wire);

  pressureSensor.settings.I2CAddress = 0x76;
  pressureSensor.settings.filter = 4;
  pressureSensor.settings.tempOverSample = 5;
  pressureSensor.settings.pressOverSample = 5;
  pressureSensor.settings.humidOverSample = 5;

  SPI.setSCK(13);

  Serial.begin(115200);

  delay(5000);

  Serial.println("Starting Radio");

  if (!rfClient.loadClientPrefixFromEEPROM())
  {
    Serial.println("Failed to load client prefix, using DFLT");
    delay(50);
  }
  else
  {
    char prefix[5] = {"0000"};
    rfClient.getClientPrefix(prefix);
    Serial.println(prefix);
  }

  rfClient.setListener(25, &resetSensor);

  rfClient.begin();

  Serial.println("Starting Sensor");

  resetSensor(1);

  delay(1000);
}

uint8_t failedCount = 0;
elapsedMillis countDown = 0;
uint8_t sensorCount = 0;
float BMEtempC = 0;
float BMEhumid = 0;
float BMEmBar = 0;
void loop()
{
  delay(10);
  rfClient.listen();
  if (Serial.available())
  {
    char in = Serial.read();
    if (in == '@')
    {
      uint8_t prefix[4];
      prefix[0] = Serial.read();
      prefix[1] = Serial.read();
      prefix[2] = Serial.read();
      prefix[3] = Serial.read();
      rfClient.saveClientPrefixToEEPROM(prefix);
      Serial.println("Client Prefix saved to eeprom");
      resetSensor(2);
    }
    else if (in == '#')
    {
      uint8_t devNum = Serial.read() - '0';
      char input[20] = {0};
      uint8_t i = 0;
      while (i < 29)
      {
        input[i] = Serial.read();
        if (input[i] == '\n' || input[i] == '\r')
        {
          break;
        }
        i++;
      }
      input[i] = '\0';
      Serial.print("Sending command:");
      Serial.print(input);
      Serial.print(" to device ");
      Serial.print(devNum);
      switch (devNum)
      {

      case 0:
        phSensor.sendCommand(input, input);
        Serial.println(" = ph");
        break;
      case 1:
        salinitySensor.sendCommand(input, input);
        Serial.println(" = salinity");
        break;
      case 2:
        orpSensor.sendCommand(input, input);
        Serial.println(" = orp");
        break;
      case 3:
        doSensor.sendCommand(input, input);
        Serial.println(" = do");
        break;
      default:
        Serial.println(" = unknown");
      }

      Serial.print("Response:");
      Serial.println(input);
    }
    else
    {
      while (Serial.available())
        Serial.read();
    }
  }

  if (countDown >= 1000)
  {
    Serial.println("Attempting Read");
    countDown = 0;

    double in = 0;
    if (sensorCount == 0)
    {
      in = phSensor.readMeasurement();
    }
    else if (sensorCount == 1)
    {
      in = salinitySensor.readMeasurement();
    }
    else if (sensorCount == 2)
    {
      in = orpSensor.readMeasurement();
    }
    else if (sensorCount == 3)
    {
      in = doSensor.readMeasurement();
    }
    else if (sensorCount == 4)
    {
      if (co2Sensor.dataAvailable())
      {
        in = co2Sensor.getCO2();
        delay(100);
        BMEmBar = pressureSensor.readFloatPressure() / 100;
        delay(500);
        if (BMEmBar > 0)
          co2Sensor.setAmbientPressure(BMEmBar);
        delay(100);
      }
      else
      {
        Serial.println(":...CO2 not ready....");
        in = -9999;
      }
    }
    else if (sensorCount == 5)
    {
      if (airSensor.dataAvailable()) //Check to see if CCS811 has new data (it's the slowest sensor)
      {
        airSensor.readAlgorithmResults(); //Read latest from CCS811 and update tVOC and CO2 variables
        in = airSensor.getTVOC();

        BMEtempC = co2Sensor.getTemperature();
        BMEhumid = co2Sensor.getHumidity();

        delay(100);
        airSensor.setEnvironmentalData(BMEhumid, BMEtempC);
      }
      else if (airSensor.checkForStatusError()) //Check to see if CCS811 has thrown an error
      {
        Serial.print("Air sensor error:");
        Serial.println(airSensor.getErrorRegister()); //Prints whatever CSS811 error flags are detected
        in = -9999;
      }
      else
      {
        Serial.println(":...VOC not ready....");
        in = -9999;
      }
    }
    else if (sensorCount == 6)
    {
      in = BMEmBar;
    }
    else if (sensorCount == 7)
    {
      in = BMEhumid;
    }
    else if (sensorCount == 8)
    {
      in = BMEtempC * 1.8 + 32;
    }

    if (in == -9999)
    {
      //failed once;
      Serial.println("Failed Read");
      delay(1000);

      failedCount++;

      if (failedCount >= 12)
      {
        Serial.println("trying wire bus reset");
        Wire.resetBus();
      }
      if (failedCount >= 18)
      {
        failedCount = 0;
        resetSensor(2);
      }
    }
    else
    {
      // all good!
      Serial.print("Got Sensor(");
      Serial.print(sensorCount);
      Serial.print(")=");
      Serial.println(in);
      rfClient.sendDouble(sensorCount, in);
      if (sensorCount < 6)
        failedCount = 0;
    }
    sensorCount++;
    if (sensorCount == 9)
      sensorCount = 0;
  }
}
