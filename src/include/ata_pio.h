#ifndef ATA_PIO_H
#define ATA_PIO_H

#include "stdint.h"

#define ATA_SECTOR_SIZE 512

// Probe the primary master channel. Returns non-zero when a device answered
// IDENTIFY DEVICE and its capacity could be read.
int ata_pio_init(void);
int ata_pio_available(void);

// Total addressable sectors reported by IDENTIFY DEVICE (words 60/61, falling
// back to the LBA48 words 100-103 for disks larger than 128 GiB).
uint64_t ata_pio_get_capacity_sectors(void);

// Marketing model string from IDENTIFY words 27-46, byte-swapped.
const char* ata_pio_get_model(void);

// Serial number from IDENTIFY words 10-19. Empty when the drive reports none.
const char* ata_pio_get_serial(void);

// True when the drive exposes more than the 28-bit LBA maximum.
int ata_pio_has_lba48(void);

// Non-zero when the last read/write used the 48-bit path.
int ata_pio_is_lba48(void);

int ata_pio_read_sector(uint64_t lba, uint8_t* out512);
int ata_pio_write_sector(uint64_t lba, const uint8_t* in512);

#endif
