#ifndef ATA_PIO_H
#define ATA_PIO_H

#include "stdint.h"

#define ATA_SECTOR_SIZE 512

int ata_pio_init(void);
int ata_pio_available(void);
int ata_pio_read_sector(uint32_t lba, uint8_t* out512);
int ata_pio_write_sector(uint32_t lba, const uint8_t* in512);

#endif
