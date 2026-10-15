#include <multiboot2.h>
#include <string.h>

// Multiboot 2 information parsing.
//
// GRUB2 hands over a different structure for Multiboot 2 than it does for
// Multiboot 1: a flat array of self-describing tags instead of the fixed
// multiboot_info layout. Everything the kernel actually reads out of the boot
// information lives in the Multiboot 1 layout (the framebuffer descriptor that
// graphics_use_multiboot_framebuffer() validates), so this module walks the
// Multiboot 2 tags once and copies the corresponding fields into a static
// multiboot_info_t. The rest of the kernel keeps using one layout and never
// learns which protocol booted it.
//
// The Multiboot 2 block is mapped by the loader into memory that stays valid
// for the life of the kernel (same guarantee Multiboot 1 made), and the kernel
// identity-maps it, so tags are read in place.

static multiboot_info_t mb2_info;
static const char* mb2_protocol = "unknown";

// Read the little-endian 32-bit tag header fields. Tags are aligned to 4 bytes
// and x86 tolerates the unaligned access, but keep it explicit so the intent
// is not left to the compiler.
static uint32_t mb2_read32(const uint8_t* p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint64_t mb2_read64(const uint8_t* p) {
    return (uint64_t)mb2_read32(p) | ((uint64_t)mb2_read32(p + 4) << 32);
}

multiboot_info_t* multiboot2_build(uint32_t magic, void* info) {
    // Multiboot 1 handover: the pointer already is the structure the kernel
    // expects, so pass it straight through.
    if (magic == MULTIBOOT1_INFO_MAGIC) {
        mb2_protocol = "Multiboot 1";
        return (multiboot_info_t*)info;
    }

    if (magic != MULTIBOOT2_INFO_MAGIC || !info) {
        return 0;
    }
    mb2_protocol = "Multiboot 2";

    memset(&mb2_info, 0, sizeof(mb2_info));

    const uint8_t* block = (const uint8_t*)info;
    uint32_t total_size = mb2_read32(block);
    // The tag list must follow the 8-byte fixed header. A block smaller than
    // that is malformed: report "no information" rather than walking off the
    // end of the structure the loader gave us.
    if (total_size < 8) {
        return 0;
    }

    const uint8_t* end = block + total_size;
    const uint8_t* tag = block + 8;

    while (tag + 8 <= end) {
        uint32_t type = mb2_read32(tag);
        uint32_t size = mb2_read32(tag + 4);
        if (size < 8 || (tag + size) > end) {
            break;  // Malformed tag: stop rather than run past the block.
        }
        const uint8_t* data = tag + 8;

        switch (type) {
            case MB2_TAG_CMDLINE:
                // The Multiboot 1 field is a pointer to a NUL-terminated
                // string, and so is the tag payload.
                mb2_info.flags |= MULTIBOOT_FLAG_CMDLINE;
                mb2_info.cmdline = (uint32_t)(uintptr_t)data;
                break;

            case MB2_TAG_BOOT_LOADER_NAME:
                mb2_info.flags |= MULTIBOOT_FLAG_LOADER;
                mb2_info.boot_loader_name = (uint32_t)(uintptr_t)data;
                break;

            case MB2_TAG_BASIC_MEMINFO:
                mb2_info.flags |= MULTIBOOT_FLAG_MEM;
                mb2_info.mem_lower = mb2_read32(data);
                mb2_info.mem_upper = mb2_read32(data + 4);
                break;

            case MB2_TAG_MMAP:
                // Multiboot 2 entries are 20-byte (base, length, type) records,
                // not the 20-byte Multiboot 1 records the legacy fields
                // describe, so only the extent is reported. Nothing in the
                // kernel walks the Multiboot 1 map today.
                mb2_info.flags |= MULTIBOOT_FLAG_MMAP;
                mb2_info.mmap_length = size - 8;
                mb2_info.mmap_addr = (uint32_t)(uintptr_t)data;
                break;

            case MB2_TAG_ELF_SECTIONS:
                // Multiboot 1 stores this as a table of four words; record the
                // section table pointer in that slot. It is informational.
                mb2_info.flags |= MULTIBOOT_FLAG_ELF;
                mb2_info.syms[0] = 0;
                mb2_info.syms[1] = 0;
                mb2_info.syms[2] = 0;
                mb2_info.syms[3] = (uint32_t)(uintptr_t)data;
                break;

            case MB2_TAG_FRAMEBUFFER: {
                uint64_t addr = mb2_read64(data);
                uint32_t pitch = mb2_read32(data + 8);
                uint32_t width = mb2_read32(data + 12);
                uint32_t height = mb2_read32(data + 16);
                uint8_t bpp = data[20];
                uint8_t type_id = data[21];
                // Tag payload has at least 22 bytes; guard against a truncated
                // one so the framebuffer descriptor is never half-filled.
                if (size < 8 + 22) {
                    break;
                }
                mb2_info.flags |= MULTIBOOT_FLAG_FB;
                mb2_info.framebuffer_addr = addr;
                mb2_info.framebuffer_pitch = pitch;
                mb2_info.framebuffer_width = width;
                mb2_info.framebuffer_height = height;
                mb2_info.framebuffer_bpp = bpp;
                mb2_info.framebuffer_type = type_id;
                break;
            }

            default:
                break;
        }

        // Tags start on an 8-byte boundary; the size field excludes padding.
        tag += (size + 7u) & ~7u;
    }

    return &mb2_info;
}

const char* multiboot2_protocol_name(void) {
    return mb2_protocol;
}
