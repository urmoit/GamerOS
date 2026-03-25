#include <ata_pio.h>
#include <ports.h>

#define ATA_IO_BASE      0x1F0
#define ATA_REG_DATA     (ATA_IO_BASE + 0)
#define ATA_REG_SECCNT   (ATA_IO_BASE + 2)
#define ATA_REG_LBA0     (ATA_IO_BASE + 3)
#define ATA_REG_LBA1     (ATA_IO_BASE + 4)
#define ATA_REG_LBA2     (ATA_IO_BASE + 5)
#define ATA_REG_HDDEVSEL (ATA_IO_BASE + 6)
#define ATA_REG_STATUS   (ATA_IO_BASE + 7)
#define ATA_REG_COMMAND  (ATA_IO_BASE + 7)

#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30
#define ATA_CMD_CACHE_FLUSH    0xE7
#define ATA_CMD_IDENTIFY       0xEC

#define ATA_SR_ERR  0x01
#define ATA_SR_DRQ  0x08
#define ATA_SR_DF   0x20
#define ATA_SR_DRDY 0x40
#define ATA_SR_BSY  0x80

static int g_ata_available = 0;

static int ata_wait_not_busy(void) {
    for (int i = 0; i < 200000; i++) {
        uint8_t st = inb(ATA_REG_STATUS);
        if (!(st & ATA_SR_BSY)) return 1;
    }
    return 0;
}

static int ata_wait_drq(void) {
    for (int i = 0; i < 200000; i++) {
        uint8_t st = inb(ATA_REG_STATUS);
        if (st & ATA_SR_ERR) return 0;
        if (st & ATA_SR_DF) return 0;
        if ((st & ATA_SR_BSY) == 0 && (st & ATA_SR_DRQ)) return 1;
    }
    return 0;
}

int ata_pio_available(void) {
    return g_ata_available;
}

int ata_pio_init(void) {
    // VMware-safe fallback: keep the higher-level filesystem online using the
    // RAM-backed storage path until ATA probing is hardened further.
    g_ata_available = 0;
    return 0;

    // Select primary master and clear taskfile registers.
    outb(ATA_REG_HDDEVSEL, 0xA0);
    io_wait();
    outb(ATA_REG_SECCNT, 0);
    outb(ATA_REG_LBA0, 0);
    outb(ATA_REG_LBA1, 0);
    outb(ATA_REG_LBA2, 0);
    outb(ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();

    uint8_t st = inb(ATA_REG_STATUS);
    if (st == 0 || st == 0xFF) {
        g_ata_available = 0;
        return 0;
    }
    if (!ata_wait_not_busy()) {
        g_ata_available = 0;
        return 0;
    }
    st = inb(ATA_REG_STATUS);
    if (st & ATA_SR_ERR) {
        g_ata_available = 0;
        return 0;
    }
    if (!(st & ATA_SR_DRDY)) {
        g_ata_available = 0;
        return 0;
    }
    if (!ata_wait_drq()) {
        g_ata_available = 0;
        return 0;
    }

    // Drain IDENTIFY data words.
    for (int i = 0; i < 256; i++) {
        (void)inw(ATA_REG_DATA);
    }
    g_ata_available = 1;
    return 1;
}

int ata_pio_read_sector(uint32_t lba, uint8_t* out512) {
    if (!g_ata_available || !out512) return 0;
    if (lba > 0x0FFFFFFF) return 0;
    if (!ata_wait_not_busy()) return 0;

    outb(ATA_REG_HDDEVSEL, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_REG_SECCNT, 1);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    if (!ata_wait_drq()) return 0;

    for (int i = 0; i < 256; i++) {
        uint16_t w = inw(ATA_REG_DATA);
        out512[i * 2] = (uint8_t)(w & 0xFF);
        out512[i * 2 + 1] = (uint8_t)(w >> 8);
    }
    return 1;
}

int ata_pio_write_sector(uint32_t lba, const uint8_t* in512) {
    if (!g_ata_available || !in512) return 0;
    if (lba > 0x0FFFFFFF) return 0;
    if (!ata_wait_not_busy()) return 0;

    outb(ATA_REG_HDDEVSEL, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_REG_SECCNT, 1);
    outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    if (!ata_wait_drq()) return 0;

    for (int i = 0; i < 256; i++) {
        uint16_t w = (uint16_t)in512[i * 2] | ((uint16_t)in512[i * 2 + 1] << 8);
        outw(ATA_REG_DATA, w);
    }

    outb(ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (!ata_wait_not_busy()) return 0;
    return 1;
}
