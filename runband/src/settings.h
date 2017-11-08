#ifndef DH_SETTINGS_H__
#define DH_SETTINGS_H__
#include "flash.h"
#include <TinyGPS++.h>

#define SETTINGS_SECTOR_NUM 0x0000

#define LIGHT_MODE 0
#define LIGHT_R 1
#define LIGHT_G 2
#define LIGHT_B 3
#define LIGHT_DENSITY 4
#define PRG_SELECTED 16
#define PRG_RGB_START 32

class Settings
{

public:
  Settings(Flash *flash);
  void loadSettings(void);
  void saveSettings(void);

  uint8_t getLightMode() { return flash->pages[LIGHT_MODE]; }
  void setLightMode(uint8_t mode) { flash->pages[LIGHT_MODE] = mode; }

  uint8_t getSelectedProgress() { return flash->pages[PRG_SELECTED]; }
  void setSelectedProgress(uint8_t selected) { flash->pages[PRG_SELECTED] = selected; }

  uint8_t getLightsRed(void) { return flash->pages[LIGHT_R]; }
  uint8_t getLightsGreen(void) { return flash->pages[LIGHT_G]; }
  uint8_t getLightsBlue(void) { return flash->pages[LIGHT_B]; }

  void setLightsRed(uint8_t r) { flash->pages[LIGHT_R] = r; }
  void setLightsGreen(uint8_t g) { flash->pages[LIGHT_G] = g; }
  void setLightsBlue(uint8_t b) { flash->pages[LIGHT_B] = b; }

  uint8_t *getProgressColor(uint8_t index)
  {
    return &(flash->pages[PRG_RGB_START + index * 3]);
  }

private:
  Flash *flash = 0;
};
#endif
