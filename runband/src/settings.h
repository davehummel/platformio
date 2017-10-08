#ifndef DH_SETTINGS_H__
#define DH_SETTINGS_H__
#include "Arduino.h"
#include <SPI.h>
#include "flashS25FL127_spi.h"
#include <TinyGPS++.h>

#define SETTINGS_PAGE_NUM 0x0001

#define LIGHT_MODE 0
#define LIGHT_R 1
#define LIGHT_G 2
#define LIGHT_B 3
#define LIGHT_DENSITY 4
#define PRG_SELECTED 16
#define PRG_RGB_START 32

class Settings {
public:
  bool init(void);
  void loadSettings(void);
  void saveSettings(void);

 uint8_t getLightMode(){ return settingsPage[LIGHT_MODE];}
 void setLightMode(uint8_t mode){settingsPage[LIGHT_MODE] = mode;}

 uint8_t getSelectedProgress(){ return settingsPage[PRG_SELECTED];}
 void setSelectedProgress(uint8_t selected){settingsPage[PRG_SELECTED] = selected;}

  uint8_t getLightsRed(void) { return settingsPage[LIGHT_R]; }
  uint8_t getLightsGreen(void) { return settingsPage[LIGHT_G]; }
  uint8_t getLightsBlue(void) { return settingsPage[LIGHT_B]; }

  void setLightsRed(uint8_t r) {  settingsPage[LIGHT_R] = r; }
  void setLightsGreen(uint8_t g) {  settingsPage[LIGHT_G] = g; }
  void setLightsBlue(uint8_t b) {  settingsPage[LIGHT_B] = b; }

  uint8_t* getProgressColor(uint8_t index){
    return &settingsPage[PRG_RGB_START+index*3];
  }



private:

  uint8_t settingsPage[256] = {0};
};

#endif
