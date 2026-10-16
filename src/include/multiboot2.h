#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include "stdint.h"
#include "multiboot.h"

// Multiboot 2 boot protocol constants (www.gnu.org/software/grub/manual/multiboot2).
#define MULTIBOOT2_MAGIC      0xE85250D6u
#define MULTIBOOT2_ARCH_I386  0x00000000u

// Value the loader puts in EAX when it passes a Multiboot 2 information block.
#define MULTIBOOT2_INFO_MAGIC 0x36D76289u

// Multiboot 1 handover magic, kept here so one caller can test both.
#define MULTIBOOT1_INFO_MAGIC 0x2BADB002u

#define MB2_TAG_END                0
#define MB2_TAG_CMDLINE            1
#define MB2_TAG_BOOT_LOADER_NAME   2
#define MB2_TAG_MODULE             3
#define MB2_TAG_BASIC_MEMINFO      4
#define MB2_TAG_BOOTDEV            5
#define MB2_TAG_MMAP               6
#define MB2_TAG_VBE                7
#define MB2_TAG_FRAMEBUFFER        8
#define MB2_TAG_ELF_SECTIONS       9
#define MB2_TAG_APM               10
#define MB2_TAG_ACPI_OLD          14
#define MB2_TAG_ACPI_NEW          15

#define MB2_FRAMEBUFFER_INDEXED  0
#define MB2_FRAMEBUFFER_RGB      1
#define MB2_FRAMEBUFFER_TEXT     2

// Returns a multiboot_info_t*, or 0 for an unknown boot protocol magic.
multiboot_info_t* multiboot2_build(uint32_t magic, void* info);

const char* multiboot2_protocol_name(void);

#endif
