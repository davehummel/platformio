#include <Arduino.h>

#include <time.h>
#include <Timezone.h>
#include <SPI.h>
#include <WiFi101.h>

#include <PubSubClient.h>

TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
Timezone usCT(usCDT, usCST);

char ssid[] = "Nicolashi-24"; // your network SSID (name)
char pass[] = "fluffyship";   // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;  // the WiFi radio's status
WiFiClient wificlient;
PubSubClient mqttClient(wificlient);

#define RELAY1 23
#define RELAY2 22
#define RELAY3 21
#define RELAY4 20

#define LEVEL1IN 2
#define LEVEL2IN 3

#define CYCLE_TIME_SEC 60


#define TOP_OFF_DUR_SEC 600
#define ALK_CALC_RUN_DUR_MS 5600

#define ALK_ADD_ONCE_PER_CYCLES 240
#define CALC_ADD_ONCE_PER_CYCLES 360
#define CALC_ALK_OFFSET 1

// tstrSystemTime WiFiClass::getSystemTime()
// {

// 	tstrSystemTime systemTime;
// 	systemTime.u8Hour = 0;
// 	systemTime.u8Minute=0;
// 	systemTime.u8Second=0;
// 	systemTime.u8Day=0;
// 	systemTime.u8Month=0;
// 	systemTime.u16Year=0;

// 	_resolve = (uint32_t)&systemTime;

// 	m2m_wifi_get_sytem_time();

// 	unsigned long start = millis();
// 	while (_resolve != 0 && millis() - start < 5000) {
// 		m2m_wifi_handle_events(NULL);
// 	}

//  _resolve = 0;
// 	return systemTime;

// }

time_t requestTime()
{

  tstrSystemTime systemTime = WiFi.getSystemTime();

  Serial.print("Year:");
  Serial.println(systemTime.u16Year);

  if (systemTime.u16Year > 0 && systemTime.u16Year < 2100)
  {
    setSyncInterval(600);
    TimeElements tm;
    uint16 yr = systemTime.u16Year;
    if (yr > 99)
      yr = yr - 1970;
    else
      yr += 30;
    tm.Year = yr;
    tm.Month = systemTime.u8Month;
    tm.Day = systemTime.u8Day;
    tm.Hour = systemTime.u8Hour;
    tm.Minute = systemTime.u8Minute;
    tm.Second = systemTime.u8Second;

    return makeTime(tm);
  }
  setSyncInterval(5);
  return 0;
}

void printMacAddress(byte mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if (mac[i] < 16)
    {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0)
    {
      Serial.print(":");
    }
  }
  Serial.println();
}

void printCurrentNet()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printWiFiData()
{
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void wifiConnect()
{
  // check for the presence of the shield:
  status = WiFi.status();
  if (status == WL_NO_SHIELD)
  {
    Serial.println("WiFi shield not present");
  }

  // attempt to connect to WiFi network:
  if (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    if (status != WL_CONNECTED)
    {
      Serial.println("Failed to connect: ");
      Serial.println(status);
    }
    else
    {
      Serial.println("You're connected to the network");
      printCurrentNet();
      printWiFiData();
      Serial.println("waiting for wifi time data to load");
      WiFi.getSystemTime();
      delay(10000);
      setSyncProvider(requestTime);
      setSyncInterval(5);
    }
  }
}

void mqttConnect()
{
  if (status == WL_CONNECTED)
  {

    if (!mqttClient.connected())
    {
      Serial.print("Connecting to mqtt");
      // Create a random client ID
      String clientId = "Teensy4Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (mqttClient.connect(clientId.c_str()))
      {
        Serial.println(":connected!");
      }
      else
      {
        Serial.print("failed, rc=");
        Serial.println(mqttClient.state());
      }
    }
    else
    {
      Serial.println("MQTT already connected");
    }
  }
  else
  {
    Serial.println("Wifi not connected, not enabling mqtt");
  }
}
elapsedMillis loopTime = 0;
void setup()
{
  Serial.begin(115200);

  delay(1000);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  pinMode(LEVEL1IN, INPUT_PULLDOWN);
  pinMode(LEVEL2IN, INPUT_PULLDOWN);

  setTime(0);

  WiFi.setPins(10, 8, 9, 7);
  mqttClient.setServer("192.168.1.3", 1883);

  loopTime = 0;
}

void addAlk(bool enable)
{
  digitalWrite(RELAY2, !enable);
}

void addCalc(bool enable)
{
  digitalWrite(RELAY3, !enable);
}

void openTopoff(bool enable)
{
  digitalWrite(RELAY1, !enable);
}

elapsedMillis topoffTimer = TOP_OFF_DUR_SEC * 1000 + 1;

uint32_t runCount = 0;

void loop()
{
  wifiConnect();

  bool isWaterLow = digitalRead(LEVEL1IN);
  bool isWaterHigh = !digitalRead(LEVEL2IN);

  bool alkOn = (runCount % ALK_ADD_ONCE_PER_CYCLES == 0);

  if (alkOn)
  {
    addAlk(true);
    // Closed delay timing here to ensure no extra liquid added
    // if other things interfere with operation
    delay(ALK_CALC_RUN_DUR_MS);
    addAlk(false);
  }
  else
  {
    addAlk(false);
  }

  bool calcOn = (runCount % ALK_ADD_ONCE_PER_CYCLES == CALC_ALK_OFFSET);

  if (calcOn)
  {
    addCalc(true);
    delay(ALK_CALC_RUN_DUR_MS);
    addCalc(false);
  }
  else
  {
    addCalc(false);
  }

  bool fillOn = false;

  if (isWaterHigh)
  {
    fillOn = false;
  }
  else if (topoffTimer > TOP_OFF_DUR_SEC * 1000)
  {
    fillOn = false;
    if (isWaterLow)
    {
      fillOn = true;
      topoffTimer = 0;
    }
    else
    {
      fillOn = false;
      // Avoid rollover if not topped of for a super long time
      topoffTimer = TOP_OFF_DUR_SEC * 1000 + 1;
    }
  }
  else
  {
    fillOn = true;
  }

  openTopoff(fillOn);

  char buffer[255];

  time_t t = usCT.toLocal(now());

  if (!fillOn)
    sprintf(buffer, "Time: %d/%d/%d %.2d:%.2d:%.2d Water Low: %d Water High: %d  Topoff Off! Alk : %d Calc : %d", day(t), month(t), year(t), hour(t), minute(t), second(t), isWaterLow, isWaterHigh, alkOn, calcOn);
  else
    sprintf(buffer, "Time: %d/%d/%d %.2d:%.2d:%.2d Water Low: %d Water High: %d  Topoff On for %lu Alk : %d Calc : %d", day(t), month(t), year(t), hour(t), minute(t), second(t), isWaterLow, isWaterHigh, TOP_OFF_DUR_SEC * 1000 - topoffTimer, alkOn, calcOn);

  Serial.println(buffer);

  mqttConnect();

  if (mqttClient.connected())
  {
    mqttClient.publish("test", buffer);
  }
  else
  {
    Serial.println("MSQTT not connected");
  }

  if (loopTime > CYCLE_TIME_SEC * 1000)
  {
    Serial.print(loopTime);
    Serial.println(" = Woa too long");
  }

  while (loopTime < CYCLE_TIME_SEC * 1000){
    delay(100);
  }
  mqttClient.loop();
  loopTime = 0;
  runCount++;
}