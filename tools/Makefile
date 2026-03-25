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
        $(SRC_DIR)/graphics/vga_graphics.c \
        $(SRC_DIR)/graphics/font.c \
        $(SRC_DIR)/ui/apps/settings/settings_ui.c \
        $(SRC_DIR)/resources/wallpapers/background_wallpaper.c \
        $(SRC_DIR)/filesystem/fs.c \
        $(SRC_DIR)/graphics/cursor.c \
        $(SRC_DIR)/drivers/input/keyboard.c \
        $(SRC_DIR)/drivers/input/mouse.c \
        $(SRC_DIR)/drivers/bus/rtc.c \
        $(SRC_DIR)/drivers/bus/pic.c \
        $(SRC_DIR)/drivers/network/serial.c \
        $(SRC_DIR)/drivers/storage/ata_pio.c \
        $(SRC_DIR)/core/kernel/string.c \
        $(SRC_DIR)/core/kernel/process_model.c

# Build artifacts
ASM_OBJ = $(BUILD_DIR)/$(ARCH)/boot.o

C_OBJ = $(BUILD_DIR)/$(ARCH)/main.o \
        $(BUILD_DIR)/$(ARCH)/ports.o \
        $(BUILD_DIR)/$(ARCH)/vga_graphics.o \
        $(BUILD_DIR)/$(ARCH)/font.o \
        $(BUILD_DIR)/$(ARCH)/apps_settings_ui.o \
        $(BUILD_DIR)/$(ARCH)/background_wallpaper.o \
        $(BUILD_DIR)/$(ARCH)/fs.o \
        $(BUILD_DIR)/$(ARCH)/cursor.o \
        $(BUILD_DIR)/$(ARCH)/keyboard.o \
        $(BUILD_DIR)/$(ARCH)/mouse.o \
        $(BUILD_DIR)/$(ARCH)/rtc.o \
        $(BUILD_DIR)/$(ARCH)/pic.o \
        $(BUILD_DIR)/$(ARCH)/serial.o \
        $(BUILD_DIR)/$(ARCH)/ata_pio.o \
        $(BUILD_DIR)/$(ARCH)/string.o \
        $(BUILD_DIR)/$(ARCH)/process_model.o

OBJS = $(ASM_OBJ) $(C_OBJ)

# Output files
KERNEL_BIN = $(DIST_DIR)/$(ARCH)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/$(ARCH)/kernel.elf
ISO = $(DIST_DIR)/$(ARCH)/kernel.iso

# Tools - using Linux cross-compiler
ASM = nasm
ASMFLAGS = -f elf64

CC = x86_64-linux-gnu-gcc
CFLAGS = -m64 -mno-red-zone -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -std=c11 -Wno-unused-parameter -Wno-unused-variable -g -O2
INCLUDES = -I$(SRC_DIR)/include -I$(SRC_DIR)/ui/apps -I$(SRC_DIR)/ui/apps/settings

LD = x86_64-linux-gnu-ld
LDFLAGS = -m elf_x86_64 -T $(TARGETS_DIR)/$(ARCH)/linker.ld -nostdlib

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

# Pattern rule for C files
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/impl/%.c | $(BUILD_DIR)/$(ARCH)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Specific C file rules
$(BUILD_DIR)/$(ARCH)/main.o: $(SRC_DIR)/core/kernel/main.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ports.o: $(SRC_DIR)/core/hal/ports.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vga_graphics.o: $(SRC_DIR)/graphics/vga_graphics.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/font.o: $(SRC_DIR)/graphics/font.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/apps_settings_ui.o: $(SRC_DIR)/ui/apps/settings/settings_ui.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/background_wallpaper.o: $(SRC_DIR)/resources/wallpapers/background_wallpaper.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/fs.o: $(SRC_DIR)/filesystem/fs.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/cursor.o: $(SRC_DIR)/graphics/cursor.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/keyboard.o: $(SRC_DIR)/drivers/input/keyboard.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mouse.o: $(SRC_DIR)/drivers/input/mouse.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/rtc.o: $(SRC_DIR)/drivers/bus/rtc.c | $(BUILD_DIR)/$(ARCH)
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
$(ISO): $(KERNEL_BIN) | $(DIST_DIR)/$(ARCH)
	mkdir -p $(BUILD_DIR)/$(ARCH)/iso/boot/grub
	cp -f $(KERNEL_ELF) $(BUILD_DIR)/$(ARCH)/iso/boot/kernel.elf
	cp -f $(TARGETS_DIR)/$(ARCH)/grub.cfg $(BUILD_DIR)/$(ARCH)/iso/boot/grub/grub.cfg
	mkdir -p /tmp/gameros_iso && cp -r $(BUILD_DIR)/$(ARCH)/iso/* /tmp/gameros_iso/
	rm -f /tmp/kernel.iso
	grub-mkrescue -o /tmp/kernel.iso /tmp/gameros_iso
	cp -f /tmp/kernel.iso $@
	rm -rf /tmp/gameros_iso /tmp/kernel.iso

# Build only ISO (assumes kernel is already built)
.PHONY: build-iso
build-iso: $(ISO)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)/$(ARCH)/kernel.bin $(DIST_DIR)/$(ARCH)/kernel.elf 2>/dev/null || true
	rm -f $(DIST_DIR)/$(ARCH)/kernel.iso 2>/dev/null || true
