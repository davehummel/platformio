#include "settings.h"
#include <flash.h>

Settings::Settings(Flash *initFlash) {
  flash = initFlash;
}

void Settings::loadSettings(void) {
  flash->loadSector(SETTINGS_SECTOR_NUM);
}

void Settings::saveSettings(void) {
  flash->saveChanges();
}
