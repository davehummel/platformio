#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <SparkFun_VL53L1X_Arduino_Library.h>


const char *ssid = "Nicolashi";
const char *password = "fluffyship";

VL53L1X distanceSensor1;
VL53L1X distanceSensor2;
Adafruit_SSD1306 display;

#define MOTOR_A_IN1 14
#define MOTOR_A_IN2 27
#define MOTOR_B_IN1 26
#define MOTOR_B_IN2 25

#define SERVO_PIN_1 13

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  ArduinoOTA.setPassword("kgtnbtrp");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  Serial.println("Wifi Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Wire.begin(2, 4, 400000);

  Serial.println("Wire0 Ready");

  Wire1.begin(21, 22, 400000);

  Serial.println("Wire1 Ready");

  // if (distanceSensor1.begin(defaultAddress_VL53L1X, Wire) == false)
  //   Serial.println("Sensor1 offline!");
  // else
  //   Serial.println("Sensor1 online!");

  // if (distanceSensor2.begin(defaultAddress_VL53L1X, Wire1) == false)
  //   Serial.println("Sensor2 offline!");
  // else
  //   Serial.println("Sensor2 online!");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Clear the buffer.
  display.clearDisplay();

  int freq = 100;
  int resolution = 8;

  // ledcSetup(0, 100, 8);
  // ledcSetup(1, freq, resolution);
  // ledcSetup(2, freq, resolution);
  // ledcSetup(3, freq, resolution);

  // ledcAttachPin(MOTOR_A_IN1, 0);
  // ledcAttachPin(MOTOR_A_IN2, 1);
  // ledcAttachPin(MOTOR_B_IN1, 2);
  // ledcAttachPin(MOTOR_B_IN2, 3);

  pinMode(5, OUTPUT);

  // ledcSetup(4, 50, 16);
  // ledcAttachPin(SERVO_PIN_1, 4);
}

#define COUNT_LOW 1638
#define COUNT_HIGH 7864

void loop()
{
  bool flipper = false;
  uint8_t count = 0;

  while (true)
  {
    count++;
    if (count == 16)
      count = 0;

    // if (distanceSensor1.newDataReady() && distanceSensor2.newDataReady())
    // {
    //   int distance1 = distanceSensor1.getDistance(); //Get the result of the measurement from the sensor
    //   int distance2 = distanceSensor2.getDistance(); //Get the result of the measurement from the sensor
    //   // text display tests
    //   display.clearDisplay();
    //   display.setTextSize(1);
    //   display.setTextColor(WHITE);
    //   display.setCursor(0, 0);
    //   display.print(distance1);
    //   display.print(",");
    //   display.print(distance2);
    //   display.print("|");
    //   ledcWrite(0, distance1 > 255 ? 255 : distance1);
    //   ledcWrite(1, 1);
    //   ledcWrite(2, distance2 > 255 ? 255 : distance2);
    //   ledcWrite(3, 1);
    //   // display.print(COUNT_LOW + ((COUNT_HIGH - COUNT_LOW)/255.0)*(distance1 > 255 ? 255 : (float) distance1));
    //   // ledcWrite(4, COUNT_LOW + ((COUNT_HIGH - COUNT_LOW)/255.0)*( distance1 > 255 ? 255 : distance1));
    // }
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println(count);

    digitalWrite(5, flipper);
    display.display();
    display.clearDisplay();
    delay(200);
    ArduinoOTA.handle();
    flipper = !flipper;
  }
}