#include "mode.h"
#include <FastLED.h>
#include <RTCZero.h>

void Mode::render()
{
  switch (state)
  {
  case Start_Mode:
  {
    float batteryLevel = battery->getPower();
    leds[0] = CRGB(255 * (1.0 - batteryLevel), 255 * batteryLevel, 0);
    if (!gps->date.isValid())
    {
      leds[1] = (millis() / 500) % 2 == 0 ? CRGB::Red : CRGB::Black;
    }
    else if (!gps->location.isValid())
    {
      leds[1] = (millis() / 500) % 2 == 0 ? CRGB::Yellow : CRGB::Black;
    }
    else
    {
      leds[1] = CRGB::Green;
    }
    FastLED.show();
    break;
  }
  case Timer_Paused:
  {

    break;
  }
  case Timer_Running:
  {

    //playTimerSound();
    break;
  }
  case Progress_Running:
  {
    // storeProgress();
    // checkProgress();
    // playProgressSound();
    break;
  }
  case Progress_Paused:
  {
    // playProgressSound();
    break;
  }

  case Config_Progress_Choose:
  {
    for (uint8_t i = 0; i < 16; i++)
    {
      uint8_t *out = settings->getProgressColor(i);
      uint8_t vals[3] = {out[0], out[1], out[2]};
      if ((0 == vals[0]) && (0 == vals[1]) && (0 == vals[2]))
      {
        vals[0] = vals[1] = vals[2] = 10;
      }
      leds[i] = CRGB(vals[0], vals[1], vals[2]);
    }
    if ((millis() / 500) % 2 == 0)
    {
      leds[settings->getSelectedProgress()] = CRGB::Black;
    }
    FastLED.show();
    break;
  }
  case Config_Lights_Set_R:
  {
    if ((millis() / 500) % 2 == 0)
    {
      leds[0] = CRGB(settings->getLightsRed(), 0, 0);
    }
    else
    {
      leds[0] = CRGB(settings->getLightsRed(), settings->getLightsGreen(),
                     settings->getLightsBlue());
    }
    for (uint8_t i = 1; i < 16; i++)
    {
      leds[i] = leds[0];
    }
    FastLED.show();
    break;
  }
  case Config_Lights_Set_G:
  {
    if ((millis() / 500) % 2 == 0)
    {
      leds[0] = CRGB(0, settings->getLightsGreen(), 0);
    }
    else
    {
      leds[0] = CRGB(settings->getLightsRed(), settings->getLightsGreen(),
                     settings->getLightsBlue());
    }
    for (uint8_t i = 1; i < 16; i++)
    {
      leds[i] = leds[0];
    }
    FastLED.show();
    break;
  }
  case Config_Lights_Set_B:
  {
    if ((millis() / 500) % 2 == 0)
    {
      leds[0] = CRGB(0, 0, settings->getLightsBlue());
    }
    else
    {
      leds[0] = CRGB(settings->getLightsRed(), settings->getLightsGreen(),
                     settings->getLightsBlue());
    }
    for (uint8_t i = 1; i < 16; i++)
    {
      leds[i] = leds[0];
    }
    FastLED.show();
    break;
  }
  // case Config_Lights_Style: {

  //   FastLED.show();
  //   break;
  // }
  default:
    break;
  }
}

void Mode::input(Button_Event event)
{
  if (event == Btn_none)
    return;
  if (event == Btn1_hold)
  {
    leds[0] =
        CRGB(100 * (1.0 - battery->getPower()), 100 * battery->getPower(), 0);
    for (uint8_t i = 1; i < NUM_LEDS; i++)
    {
      leds[i] = leds[0];
      FastLED.show();
    }
    delay(1000);
    battery->off();
  }
  switch (state)
  {
  case Start_Mode:
  {
    switch (event)
    {
    case Btn1_pressed:
      showRunningLights();
      setState(Timer_Paused);
      break;
    case Btn2_pressed:
      showRunningLights();
      setState(Progress_Paused);
      break;
    case Btn2_hold_release:
      setState(Config_Progress_Choose);
      break;
    default:
      break;
    }
    break;
  }
  case Timer_Paused:
  {
    switch (event)
    {
    case Btn1_pressed:
      //startTimer();
      setState(Timer_Running);
      break;
    case Btn2_pressed:
      //startTimer();
      setState(Timer_Running);
      break;
    case Btn2_hold_release:
      setState(Config_Progress_Choose);
      break;
    default:
      break;
    }
    break;
  }
  case Timer_Running:
  {
    switch (event)
    {
    case Btn1_pressed:
      //pauseTimer();
      setState(Timer_Paused);
      break;
    case Btn2_pressed:
      // pauseTimer();
      setState(Timer_Paused);
      break;
    case Btn2_hold_release:
      //pauseTimer();
      setState(Config_Progress_Choose);
      break;
    default:
      break;
    }
    break;
  }
  case Config_Progress_Choose:
  {
    switch (event)
    {
    case Btn1_pressed:
      settings->saveSettings();
      setState(Start_Mode);
      break;
    case Btn2_pressed:
    {
      uint8_t selected = settings->getSelectedProgress();
      selected++;
      if (selected >= 16)
        selected = 0;
      settings->setSelectedProgress(selected);
      break;
    }
    case Btn2_hold_release:
      setState(Config_Lights_Set_R);
      break;
    default:
      break;
    }
    break;
  }
  case Config_Lights_Set_R:
  {
    switch (event)
    {
    case Btn1_pressed:
      settings->saveSettings();
      setState(Start_Mode);
      break;
    case Btn2_pressed:
    {
      uint16_t selected = settings->getLightsRed();
      if (selected == 0)
      {
        selected = 1;
      }
      else if (selected == 1)
      {
        selected = 2;
      }
      else if (selected == 255)
      {
        selected = 0;
      }
      else
      {
        selected = selected * 1.5;
        if (selected > 255)
        {
          selected = 255;
        }
      }
      settings->setLightsRed((uint8_t)selected);
      break;
    }
    case Btn2_hold_release:
      setState(Config_Lights_Set_G);
      break;
    default:
      break;
    }
    break;
  }
  case Config_Lights_Set_G:
  {
    switch (event)
    {
    case Btn1_pressed:
      settings->saveSettings();
      setState(Start_Mode);
      break;
    case Btn2_pressed:
    {
      uint16_t selected = settings->getLightsGreen();
      if (selected == 0)
      {
        selected = 1;
      }
      else if (selected == 1)
      {
        selected = 2;
      }
      else if (selected == 255)
      {
        selected = 0;
      }
      else
      {
        selected = selected * 1.5;
        if (selected > 255)
        {
          selected = 255;
        }
      }
      settings->setLightsGreen((uint8_t)selected);
      break;
    }
    case Btn2_hold_release:
      setState(Config_Lights_Set_B);
      break;
    default:
      break;
    }
    break;
  }
  case Config_Lights_Set_B:
  {
    switch (event)
    {
    case Btn1_pressed:
      settings->saveSettings();
      setState(Start_Mode);
      break;
    case Btn2_pressed:
    {
      uint16_t selected = settings->getLightsBlue();
      if (selected == 0)
      {
        selected = 1;
      }
      else if (selected == 1)
      {
        selected = 2;
      }
      else if (selected == 255)
      {
        selected = 0;
      }
      else
      {
        selected = selected * 1.5;
        if (selected > 255)
        {
          selected = 255;
        }
      }
      settings->setLightsBlue((uint8_t)selected);
      break;
    }
    case Btn2_hold_release:
      settings->saveSettings();
      setState(Start_Mode);
      break;
    default:
      break;
    }
    break;
  }
  default:
    break;
  }
}

void Mode::initState(void)
{

  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void Mode::showRunningLights(void)
{
  CRGB rgbVal = CRGB(settings->getLightsRed(), settings->getLightsGreen(), settings->getLightsBlue());
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = rgbVal;
  }
  FastLED.show();
}
