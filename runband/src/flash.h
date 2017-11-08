#ifndef DH_FLASH_H__
#define DH_FLASH_H__

#include <SPI.h>
#include "flashS25FL127_spi.h"

#define MAX_SECTOR 4096
#define PAGES_PER_SECTOR 16

class Flash {

public:

  bool init(void){
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
  void loadSector(uint16_t sector){
    if (sector>MAX_SECTOR)
        return;

    flash_read_pages(pages, sector*PAGES_PER_SECTOR, PAGES_PER_SECTOR);

    currentSector = sector;
    
  }

  void saveChanges(void){
    if (currentSector == MAX_SECTOR+1)
          return;
      
    // Must erase before write

    flash_erase_pages_sector(currentSector*PAGES_PER_SECTOR);

    for (uint8_t i = 0 ; i < PAGES_PER_SECTOR ; i++){
        flash_page_program(pages+i,currentSector*PAGES_PER_SECTOR+i);
    }

  }

  uint8_t pages[256*PAGES_PER_SECTOR] = {0};

private:

  uint16_t currentSector = MAX_SECTOR+1;

};

#endif
