ARCH ?= x86_64

# Directories
SRC_DIR = ../src
BUILD_DIR = ../build
DIST_DIR = ../dist
TARGETS_DIR = ../config/targets

# Core source files (working set)
ASM_SRC = $(SRC_DIR)/arch/x86_64/boot.asm

C_SRC = $(SRC_DIR)/core/kernel/main.c \
        $(SRC_DIR)/core/hal/ports.c \
        $(SRC_DIR)/core/kernel/mm.c \
        $(SRC_DIR)/core/kernel/mb2.c \
        $(SRC_DIR)/graphics/vga_graphics.c \
        $(SRC_DIR)/graphics/font.c \
        $(SRC_DIR)/resources/wallpapers/background_wallpaper.c \
        $(SRC_DIR)/ui/apps/notepad/notepad_app.c \
        $(SRC_DIR)/ui/apps/settings/settings_ui.c \
        $(SRC_DIR)/ui/markdown/markdown.c \
        $(SRC_DIR)/filesystem/fs.c \
        $(SRC_DIR)/graphics/cursor.c \
        $(SRC_DIR)/drivers/input/keyboard.c \
        $(SRC_DIR)/drivers/input/mouse.c \
        $(SRC_DIR)/drivers/bus/rtc.c \
        $(SRC_DIR)/drivers/bus/pci.c \
        $(SRC_DIR)/drivers/bus/pic.c \
        $(SRC_DIR)/drivers/bus/usb.c \
        $(SRC_DIR)/drivers/bus/xhci.c \
        $(SRC_DIR)/drivers/virtualization/vmware.c \
        $(SRC_DIR)/drivers/virtualization/vboxguest.c \
        $(SRC_DIR)/drivers/network/serial.c \
        $(SRC_DIR)/drivers/storage/ata_pio.c \
        $(SRC_DIR)/drivers/rtc/timezone.c \
        $(SRC_DIR)/core/kernel/string.c \
        $(SRC_DIR)/core/kernel/process_model.c

# Build artifacts
ASM_OBJ = $(BUILD_DIR)/$(ARCH)/boot.o

C_OBJ = $(BUILD_DIR)/$(ARCH)/main.o \
        $(BUILD_DIR)/$(ARCH)/ports.o \
        $(BUILD_DIR)/$(ARCH)/mm.o \
        $(BUILD_DIR)/$(ARCH)/mb2.o \
        $(BUILD_DIR)/$(ARCH)/vga_graphics.o \
        $(BUILD_DIR)/$(ARCH)/font.o \
        $(BUILD_DIR)/$(ARCH)/background_wallpaper.o \
        $(BUILD_DIR)/$(ARCH)/notepad_app.o \
        $(BUILD_DIR)/$(ARCH)/settings_ui.o \
        $(BUILD_DIR)/$(ARCH)/markdown.o \
        $(BUILD_DIR)/$(ARCH)/fs.o \
        $(BUILD_DIR)/$(ARCH)/cursor.o \
        $(BUILD_DIR)/$(ARCH)/keyboard.o \
        $(BUILD_DIR)/$(ARCH)/mouse.o \
        $(BUILD_DIR)/$(ARCH)/rtc.o \
        $(BUILD_DIR)/$(ARCH)/pci.o \
        $(BUILD_DIR)/$(ARCH)/pic.o \
        $(BUILD_DIR)/$(ARCH)/usb.o \
        $(BUILD_DIR)/$(ARCH)/xhci.o \
        $(BUILD_DIR)/$(ARCH)/vmware.o \
        $(BUILD_DIR)/$(ARCH)/vboxguest.o \
        $(BUILD_DIR)/$(ARCH)/serial.o \
        $(BUILD_DIR)/$(ARCH)/ata_pio.o \
        $(BUILD_DIR)/$(ARCH)/timezone.o \
        $(BUILD_DIR)/$(ARCH)/string.o \
        $(BUILD_DIR)/$(ARCH)/process_model.o

OBJS = $(ASM_OBJ) $(C_OBJ)

# Output files
KERNEL_BIN = $(DIST_DIR)/$(ARCH)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/$(ARCH)/kernel.elf
ISO = $(DIST_DIR)/$(ARCH)/kernel.iso

# Tools - using Linux cross-compiler
ASM = nasm
# -I$(SRC_DIR) lets boot.asm resolve %include "graphics/font.inc" regardless of
# the directory make was invoked from.
ASMFLAGS = -f elf64 -I$(SRC_DIR)

CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -mno-red-zone -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fomit-frame-pointer -Wall -Wextra -std=c11 -Wno-unused-parameter -Wno-unused-variable -O2
  INCLUDES = -I$(SRC_DIR)/include -I$(SRC_DIR)/ui/shell -I$(SRC_DIR)/ui/apps/notepad -I$(SRC_DIR)/ui/apps/settings -I$(SRC_DIR)/ui/markdown

LD = x86_64-linux-gnu-ld
LDFLAGS = -m elf_x86_64 -T $(TARGETS_DIR)/$(ARCH)/linker.ld -nostdlib -O2

# Default target
.PHONY: all
all: build-$(ARCH)

# Build kernel for x86_64
.PHONY: build-x86_64
build-x86_64: clean $(ISO)
	@echo "Build complete!"

# Create directories
$(BUILD_DIR)/$(ARCH):
	mkdir -p $(BUILD_DIR)/$(ARCH)

$(DIST_DIR)/$(ARCH):
	mkdir -p $(DIST_DIR)/$(ARCH)

# Assembly files
$(BUILD_DIR)/$(ARCH)/boot.o: $(SRC_DIR)/arch/x86_64/boot.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

# Pattern rule for C sources. Note: the old rule pointed at $(SRC_DIR)/impl/,
# a directory that no longer exists, and every object also had an explicit rule
# below, so it could never fire.
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/$(ARCH)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Specific C file rules
$(BUILD_DIR)/$(ARCH)/main.o: $(SRC_DIR)/core/kernel/main.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ports.o: $(SRC_DIR)/core/hal/ports.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mm.o: $(SRC_DIR)/core/kernel/mm.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mb2.o: $(SRC_DIR)/core/kernel/mb2.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vga_graphics.o: $(SRC_DIR)/graphics/vga_graphics.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/font.o: $(SRC_DIR)/graphics/font.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/background_wallpaper.o: $(SRC_DIR)/resources/wallpapers/background_wallpaper.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

  $(BUILD_DIR)/$(ARCH)/notepad_app.o: $(SRC_DIR)/ui/apps/notepad/notepad_app.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

  # Settings owns its own pages, state and changelog; it includes <settings_ui.h>
  # from its own directory.
  $(BUILD_DIR)/$(ARCH)/settings_ui.o: $(SRC_DIR)/ui/apps/settings/settings_ui.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
  $(BUILD_DIR)/$(ARCH)/markdown.o: $(SRC_DIR)/ui/markdown/markdown.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -I$(SRC_DIR)/ui/markdown -c -o $@ $<
$(BUILD_DIR)/$(ARCH)/fs.o: $(SRC_DIR)/filesystem/fs.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/cursor.o: $(SRC_DIR)/graphics/cursor.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/keyboard.o: $(SRC_DIR)/drivers/input/keyboard.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mouse.o: $(SRC_DIR)/drivers/input/mouse.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/pci.o: $(SRC_DIR)/drivers/bus/pci.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/usb.o: $(SRC_DIR)/drivers/bus/usb.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/xhci.o: $(SRC_DIR)/drivers/bus/xhci.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vmware.o: $(SRC_DIR)/drivers/virtualization/vmware.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vboxguest.o: $(SRC_DIR)/drivers/virtualization/vboxguest.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

  $(BUILD_DIR)/$(ARCH)/rtc.o: $(SRC_DIR)/drivers/bus/rtc.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

  $(BUILD_DIR)/$(ARCH)/timezone.o: $(SRC_DIR)/drivers/rtc/timezone.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


$(BUILD_DIR)/$(ARCH)/pic.o: $(SRC_DIR)/drivers/bus/pic.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/serial.o: $(SRC_DIR)/drivers/network/serial.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ata_pio.o: $(SRC_DIR)/drivers/storage/ata_pio.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/string.o: $(SRC_DIR)/core/kernel/string.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/process_model.o: $(SRC_DIR)/core/kernel/process_model.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Link kernel
$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create binary
$(KERNEL_BIN): $(KERNEL_ELF) | $(DIST_DIR)/$(ARCH)
	x86_64-linux-gnu-objcopy -O binary $< $@

# Create ISO
# Stage inside the build tree rather than /tmp: the ISO is produced from a Unix
# shell, but /tmp may be read-only or absent under some Windows setups.
ISO_STAGE = $(BUILD_DIR)/$(ARCH)/iso_stage
ISO_OUT = $(BUILD_DIR)/$(ARCH)/kernel.iso

$(ISO): $(KERNEL_BIN) | $(DIST_DIR)/$(ARCH)
	rm -rf $(ISO_STAGE)
	mkdir -p $(ISO_STAGE)/boot/grub
	cp -f $(KERNEL_ELF) $(ISO_STAGE)/boot/kernel.elf
	cp -f $(TARGETS_DIR)/$(ARCH)/grub.cfg $(ISO_STAGE)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_OUT) $(ISO_STAGE)
	mkdir -p $(DIST_DIR)/$(ARCH)
	cp -f $(ISO_OUT) $@
	rm -rf $(ISO_STAGE) $(ISO_OUT)

# Build only ISO (assumes kernel is already built)
.PHONY: build-iso
build-iso: $(ISO)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)/$(ARCH)/kernel.bin $(DIST_DIR)/$(ARCH)/kernel.elf 2>/dev/null || true
	rm -f $(DIST_DIR)/$(ARCH)/kernel.iso 2>/dev/null || true
