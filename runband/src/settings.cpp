#include "settings.h"
#include "flashS25FL127_spi.h"
#include <SPI.h>

bool Settings::init() {
  flash_init();
  flash_hard_reset();
  flash_read_status();
  unsigned char id_tab[32];
  flash_read_id(id_tab);
  if (id_tab[0] != 1 || id_tab[1] != 32 || id_tab[2] != 24 || id_tab[3] != 77)
    return false;
  else
    return true;
}

void Settings::loadSettings(void) {
  flash_read_pages(settingsPage, SETTINGS_PAGE_NUM, 1);
}

void Settings::saveSettings(void) {
  flash_page_program(settingsPage, SETTINGS_PAGE_NUM);
  delay(1);
  flash_read_pages(settingsPage, SETTINGS_PAGE_NUM, 1);
}
