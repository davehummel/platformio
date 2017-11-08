#ifndef DH_MODE_H__
#define DH_MODE_H__

#include "Arduino.h"
#include <RTCZero.h>
#include <FastLED.h>
#include <TinyGPS++.h>
#include "Battery.h"
#include "Settings.h"

#define NUM_LEDS 18

enum States
{
  Start_Mode,
  Progress_Paused,
  Progress_Running,
  Timer_Paused,
  Timer_Running,
  Config_Lights_Set_R,
  Config_Lights_Set_G,
  Config_Lights_Set_B,
  Config_Lights_Style,
  Config_Lights_Density,
  Config_Progress_Choose,
  Config_Progress_Save,
  Config_Progress_Save_Confirm
};

enum Button_Event
{
  Btn_none,
  Btn1_pressed,
  Btn1_hold,
  Btn1_hold_release,
  Btn2_pressed,
  Btn2_hold,
  Btn2_hold_release
};

class Mode
{
public:
  void setResources(Settings *settingsIn, RTCZero *rtcIn, TinyGPSPlus *gpsIn, CRGB *ledsIn, Battery *batteryIn)
  {
    settings = settingsIn;
    rtc = rtcIn;
    gps = gpsIn;
    leds = ledsIn;
    battery = batteryIn;
  }
  void setState(States newState)
  {
    state = newState;
    initState();
  }
  void render();
  void input(Button_Event event);

private:
  void initState(void);
  void showRunningLights(void);

  States state;
  Settings *settings;
  CRGB *leds;
  RTCZero *rtc;
  TinyGPSPlus *gps;
  Battery *battery;
};

#endif
