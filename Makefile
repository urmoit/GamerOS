ARCH ?= x86_64

# Directories
SRC_DIR = src
BUILD_DIR = build
DIST_DIR = dist
TARGETS_DIR = targets

# Source files
ASM_SRC = $(SRC_DIR)/impl/x86_64/boot.asm \
        $(SRC_DIR)/impl/x86_64/context_switch.asm
C_SRC = $(SRC_DIR)/impl/kernel/main.c \
        $(SRC_DIR)/impl/x86_64/vga_graphics.c \
        $(SRC_DIR)/impl/x86_64/font.c \
        $(SRC_DIR)/impl/x86_64/ui.c \
        $(SRC_DIR)/impl/x86_64/rtc.c \
        $(SRC_DIR)/impl/x86_64/window.c \
        $(SRC_DIR)/impl/kernel/fs.c \
        $(SRC_DIR)/impl/kernel/string.c \
        $(SRC_DIR)/impl/kernel/mm.c \
        $(SRC_DIR)/impl/kernel/scheduler.c \
        $(SRC_DIR)/impl/x86_64/keyboard.c \
        $(SRC_DIR)/impl/x86_64/pic.c \
        $(SRC_DIR)/impl/x86_64/mouse.c

# Build artifacts
ASM_OBJ = $(BUILD_DIR)/$(ARCH)/boot.o \
        $(BUILD_DIR)/$(ARCH)/context_switch.o
C_OBJ = $(BUILD_DIR)/$(ARCH)/main.o \
        $(BUILD_DIR)/$(ARCH)/vga_graphics.o \
        $(BUILD_DIR)/$(ARCH)/font.o \
        $(BUILD_DIR)/$(ARCH)/ui.o \
        $(BUILD_DIR)/$(ARCH)/rtc.o \
        $(BUILD_DIR)/$(ARCH)/window.o \
        $(BUILD_DIR)/$(ARCH)/fs.o \
        $(BUILD_DIR)/$(ARCH)/string.o \
        $(BUILD_DIR)/$(ARCH)/mm.o \
        $(BUILD_DIR)/$(ARCH)/scheduler.o \
        $(BUILD_DIR)/$(ARCH)/keyboard.o \
        $(BUILD_DIR)/$(ARCH)/pic.o \
        $(BUILD_DIR)/$(ARCH)/mouse.o
OBJS = $(ASM_OBJ) $(C_OBJ)

# Output files
KERNEL_BIN = $(DIST_DIR)/$(ARCH)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/$(ARCH)/kernel.elf
ISO = $(DIST_DIR)/$(ARCH)/kernel.iso

# Compiler flags
ASM = nasm
ASMFLAGS = -f elf64

CC = gcc
CFLAGS = -m64 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -std=c11
INCLUDES = -I$(SRC_DIR)/intf

LD = ld
LDFLAGS = -m elf_x86_64 -T $(TARGETS_DIR)/$(ARCH)/linker.ld -nostdlib

# Default target
.PHONY: all
all: build-$(ARCH)

# Build kernel for x86_64
.PHONY: build-x86_64
build-x86_64: $(ISO)

# Create directories
$(BUILD_DIR)/$(ARCH):
	mkdir -p $(BUILD_DIR)/$(ARCH)

$(DIST_DIR)/$(ARCH):
	mkdir -p $(DIST_DIR)/$(ARCH)

# Compile assembly
$(ASM_OBJ): $(ASM_SRC) | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

# Compile C
$(BUILD_DIR)/$(ARCH)/main.o: $(SRC_DIR)/impl/kernel/main.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vga_graphics.o: $(SRC_DIR)/impl/x86_64/vga_graphics.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/font.o: $(SRC_DIR)/impl/x86_64/font.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ui.o: $(SRC_DIR)/impl/x86_64/ui.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/rtc.o: $(SRC_DIR)/impl/x86_64/rtc.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/window.o: $(SRC_DIR)/impl/x86_64/window.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/fs.o: $(SRC_DIR)/impl/kernel/fs.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/string.o: $(SRC_DIR)/impl/kernel/string.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mm.o: $(SRC_DIR)/impl/kernel/mm.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/scheduler.o: $(SRC_DIR)/impl/kernel/scheduler.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/keyboard.o: $(SRC_DIR)/impl/x86_64/keyboard.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/pic.o: $(SRC_DIR)/impl/x86_64/pic.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mouse.o: $(SRC_DIR)/impl/x86_64/mouse.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/context_switch.o: $(SRC_DIR)/impl/x86_64/context_switch.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

# Link kernel
$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create binary
$(KERNEL_BIN): $(KERNEL_ELF) | $(DIST_DIR)/$(ARCH)
	objcopy -O binary $< $@

# Create ISO
$(ISO): $(KERNEL_BIN) | $(DIST_DIR)/$(ARCH)
	mkdir -p $(BUILD_DIR)/$(ARCH)/iso/boot/grub
	cp $(KERNEL_ELF) $(BUILD_DIR)/$(ARCH)/iso/boot/kernel.elf
	cp $(TARGETS_DIR)/$(ARCH)/grub.cfg $(BUILD_DIR)/$(ARCH)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/$(ARCH)/iso

# Build only ISO (assumes kernel is already built)
.PHONY: build-iso
build-iso: $(ISO)

$(ISO): $(KERNEL_BIN)
	mkdir -p $(BUILD_DIR)/$(ARCH)/iso/boot/grub
	cp $(KERNEL_ELF) $(BUILD_DIR)/$(ARCH)/iso/boot/kernel.elf
	cp $(TARGETS_DIR)/$(ARCH)/grub.cfg $(BUILD_DIR)/$(ARCH)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/$(ARCH)/iso

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)/$(ARCH)/kernel.bin $(DIST_DIR)/$(ARCH)/kernel.elf 2>/dev/null || true
	rm -f $(DIST_DIR)/$(ARCH)/kernel.iso 2>/dev/null || true



