#include <ata_pio.h>
#include <ports.h>
#include <string.h>


// Legacy ATA (IDE) PIO driver, primary channel, master device.
//
// Sectors above 128 GiB need the LBA48 command set (0x24/0x34) rather than the
// 28-bit form, so both paths are implemented and selected per request from the
// capacity discovered at IDENTIFY time.

#define ATA_IO_BASE      0x1F0
#define ATA_REG_DATA     (ATA_IO_BASE + 0)
#define ATA_REG_SECCNT   (ATA_IO_BASE + 2)
#define ATA_REG_LBA0     (ATA_IO_BASE + 3)
#define ATA_REG_LBA1     (ATA_IO_BASE + 4)
#define ATA_REG_LBA2     (ATA_IO_BASE + 5)
#define ATA_REG_HDDEVSEL (ATA_IO_BASE + 6)
#define ATA_REG_STATUS   (ATA_IO_BASE + 7)
#define ATA_REG_COMMAND  (ATA_IO_BASE + 7)

#define ATA_CMD_READ_SECTORS      0x20
#define ATA_CMD_WRITE_SECTORS     0x30
#define ATA_CMD_READ_SECTORS_EXT  0x24
#define ATA_CMD_WRITE_SECTORS_EXT 0x34
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_IDENTIFY          0xEC

#define ATA_SR_ERR  0x01
#define ATA_SR_DRQ  0x08
#define ATA_SR_DF   0x20
#define ATA_SR_DRDY 0x40
#define ATA_SR_DSC  0x10
#define ATA_SR_BSY  0x80

// Highest LBA reachable with the 28-bit task file: 0x0FFFFFFF sectors (128 GiB).
#define ATA_LBA28_MAX 0x0FFFFFFFULL

#define ATA_TIMEOUT 200000

static int g_ata_available = 0;
static int g_ata_lba48 = 0;
static uint64_t g_ata_capacity_sectors = 0;
static char g_ata_model[41];
static char g_ata_serial[21];

static int ata_wait_not_busy(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t st = inb(ATA_REG_STATUS);
        if (!(st & ATA_SR_BSY)) return 1;
    }
    return 0;
}

static int ata_wait_drq(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t st = inb(ATA_REG_STATUS);
        if (st & ATA_SR_ERR) return 0;
        if (st & ATA_SR_DF) return 0;
        if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ)) return 1;
    }
    return 0;
}

// IDENTIFY strings are stored as two characters per word, high byte first, and
// padded with spaces. Copy them out as a trimmed NUL-terminated string.
static void ata_copy_ident_string(char* dest, size_t dest_size,
                                  const uint8_t* words, int first_word, int word_count) {
    size_t out = 0;
    for (int w = 0; w < word_count; w++) {
        uint16_t word = (uint16_t)words[(first_word + w) * 2] |
                        ((uint16_t)words[(first_word + w) * 2 + 1] << 8);
        char pair[2];
        pair[0] = (char)(word >> 8);   // high byte first
        pair[1] = (char)(word & 0xFF);
        for (int k = 0; k < 2; k++) {
            if (out + 1 >= dest_size) { dest[out] = 0; return; }
            dest[out++] = pair[k];
        }
    }
    dest[out] = 0;
    // Trim trailing spaces so the Explorer shows a clean name.
    while (out > 0 && dest[out - 1] == ' ') dest[--out] = 0;
    // Collapse leading spaces as well.
    if (dest[0] == ' ') {
        size_t i = 0;
        while (dest[i] == ' ') i++;
        size_t j = 0;
        while (dest[i]) dest[j++] = dest[i++];
        dest[j] = 0;
    }
}

int ata_pio_available(void) {
    return g_ata_available;
}

uint64_t ata_pio_get_capacity_sectors(void) {
    return g_ata_capacity_sectors;
}

const char* ata_pio_get_model(void) {
    return g_ata_model;
}

const char* ata_pio_get_serial(void) {
    return g_ata_serial;
}

int ata_pio_has_lba48(void) {
    return g_ata_lba48;
}

int ata_pio_is_lba48(void) {
    return g_ata_lba48;
}

int ata_pio_init(void) {
    g_ata_available = 0;
    g_ata_lba48 = 0;
    g_ata_capacity_sectors = 0;
    g_ata_model[0] = 0;
    g_ata_serial[0] = 0;

    uint8_t identify[512];

    // Probe all four channels/drives rather than assuming 0xA0 exists, so a
    // machine whose disk sits on the secondary channel still works.
    for (int dev = 0; dev < 4; dev++) {
        uint8_t devsel = (uint8_t)(0xA0 | (dev << 4));
        outb(ATA_REG_HDDEVSEL, devsel);
        io_wait();

        // Software reset and wait for the signature. Some controllers never
        // assert DRQ for the signature phase, so a failure here is not fatal:
        // fall through and try IDENTIFY anyway.
        outb(ATA_REG_SECCNT, 0x01);
        outb(ATA_REG_LBA1, 0x00);
        outb(ATA_REG_HDDEVSEL, devsel);
        outb(ATA_REG_COMMAND, 0x08); // DEVICE RESET
        io_wait();

        int sig_ready = ata_wait_not_busy() && ata_wait_drq();
        if (sig_ready) {
            for (int i = 0; i < 256; i++) (void)inw(ATA_REG_DATA);  // discard signature
        } else {
        }

        // IDENTIFY DEVICE.
        outb(ATA_REG_HDDEVSEL, devsel);
        outb(ATA_REG_SECCNT, 0);
        outb(ATA_REG_LBA0, 0);
        outb(ATA_REG_LBA1, 0);
        outb(ATA_REG_LBA2, 0);
        outb(ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
        io_wait();

        uint8_t st = inb(ATA_REG_STATUS);
        if (st == 0xFF) continue;   // nothing on this port
        if (!ata_wait_not_busy()) continue;
        st = inb(ATA_REG_STATUS);
        if (st & ATA_SR_ERR) continue;
        if (!ata_wait_drq()) continue;

        for (int i = 0; i < 256; i++) {
            uint16_t w = inw(ATA_REG_DATA);
            identify[i * 2] = (uint8_t)(w & 0xFF);
            identify[i * 2 + 1] = (uint8_t)(w >> 8);
        }

        uint32_t lba28_low  = (uint32_t)identify[120] | ((uint32_t)identify[121] << 8);
        uint32_t lba28_high = (uint32_t)identify[122] | ((uint32_t)identify[123] << 8);
        uint64_t capacity = (uint64_t)lba28_low | ((uint64_t)lba28_high << 16);

        // Bit 12 of word 61 set means the 28-bit field is not the real count;
        // words 100-103 carry the LBA48 total instead.
        int use_lba48 = (lba28_high & 0xF000) ? 1 : 0;
        if (capacity == 0xFFFFFFFFULL || use_lba48) {
            uint32_t w100 = (uint32_t)identify[200] | ((uint32_t)identify[201] << 8);
            uint32_t w101 = (uint32_t)identify[202] | ((uint32_t)identify[203] << 8);
            uint32_t w102 = (uint32_t)identify[204] | ((uint32_t)identify[205] << 8);
            uint32_t w103 = (uint32_t)identify[206] | ((uint32_t)identify[207] << 8);
            capacity = (uint64_t)w100 | ((uint64_t)w101 << 16) |
                       ((uint64_t)w102 << 32) | ((uint64_t)w103 << 48);
        }

        if (capacity == 0) continue;

        g_ata_capacity_sectors = capacity;
        g_ata_lba48 = (capacity > ATA_LBA28_MAX) ? 1 : 0;
        ata_copy_ident_string(g_ata_model, sizeof(g_ata_model), identify, 27, 20);
        ata_copy_ident_string(g_ata_serial, sizeof(g_ata_serial), identify, 10, 10);
        g_ata_available = 1;
        return 1;
    }

    return 0;
}

// Program the task file for a single-sector request and issue the command.
static int ata_issue_read(uint64_t lba) {
    if (!ata_wait_not_busy()) return 0;

    if (lba > ATA_LBA28_MAX) {
        // LBA48: set bit 6 and the top 8 LBA bits, then re-select the drive
        // with LBA mode before writing the low 24 bits.
        outb(ATA_REG_HDDEVSEL, 0x40);
        outb(ATA_REG_SECCNT, 0x00);
        outb(ATA_REG_LBA2, (uint8_t)((lba >> 24) & 0xFF));
        outb(ATA_REG_LBA1, 0x00);
        outb(ATA_REG_LBA0, 0x00);

        outb(ATA_REG_HDDEVSEL, 0xE0);
        outb(ATA_REG_SECCNT, 0x01);
        outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
        outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
        outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
        outb(ATA_REG_COMMAND, ATA_CMD_READ_SECTORS_EXT);
    } else {
        outb(ATA_REG_HDDEVSEL, 0xE0);
        outb(ATA_REG_SECCNT, 0x01);
        outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
        outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
        outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
        outb(ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    }
    return ata_wait_drq();
}

static int ata_issue_write(uint64_t lba) {
    if (!ata_wait_not_busy()) return 0;

    if (lba > ATA_LBA28_MAX) {
        outb(ATA_REG_HDDEVSEL, 0x40);
        outb(ATA_REG_SECCNT, 0x00);
        outb(ATA_REG_LBA2, (uint8_t)((lba >> 24) & 0xFF));
        outb(ATA_REG_LBA1, 0x00);
        outb(ATA_REG_LBA0, 0x00);

        outb(ATA_REG_HDDEVSEL, 0xE0);
        outb(ATA_REG_SECCNT, 0x01);
        outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
        outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
        outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
        outb(ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS_EXT);
    } else {
        outb(ATA_REG_HDDEVSEL, 0xE0);
        outb(ATA_REG_SECCNT, 0x01);
        outb(ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
        outb(ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
        outb(ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
        outb(ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    }
    return ata_wait_drq();
}

int ata_pio_read_sector(uint64_t lba, uint8_t* out512) {
    if (!g_ata_available || !out512) return 0;
    if (lba >= g_ata_capacity_sectors) return 0;
    if (!ata_issue_read(lba)) return 0;

    for (int i = 0; i < 256; i++) {
        uint16_t w = inw(ATA_REG_DATA);
        out512[i * 2] = (uint8_t)(w & 0xFF);
        out512[i * 2 + 1] = (uint8_t)(w >> 8);
    }
    return 1;
}

int ata_pio_write_sector(uint64_t lba, const uint8_t* in512) {
    if (!g_ata_available || !in512) return 0;
    if (lba >= g_ata_capacity_sectors) return 0;
    if (!ata_issue_write(lba)) return 0;

    for (int i = 0; i < 256; i++) {
        uint16_t w = (uint16_t)in512[i * 2] | ((uint16_t)in512[i * 2 + 1] << 8);
        outw(ATA_REG_DATA, w);
    }

    outb(ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (!ata_wait_not_busy()) return 0;
    return 1;
}
