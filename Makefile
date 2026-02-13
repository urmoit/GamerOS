ARCH ?= x86_64

# Directories
SRC_DIR = src
BUILD_DIR = build
DIST_DIR = dist
TARGETS_DIR = targets

# Core source files (working set)
ASM_SRC = $(SRC_DIR)/impl/x86_64/boot.asm \
          $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.asm \
          $(SRC_DIR)/impl/kernel_mode/hal/cpu/context_switch.asm

C_SRC = $(SRC_DIR)/impl/kernel/main.c \
        $(SRC_DIR)/impl/graphics/vga_graphics.c \
        $(SRC_DIR)/impl/graphics/font.c \
        $(SRC_DIR)/impl/graphics/cursor.c \
        $(SRC_DIR)/impl/drivers/keyboard.c \
        $(SRC_DIR)/impl/drivers/mouse.c \
        $(SRC_DIR)/impl/drivers/rtc.c \
        $(SRC_DIR)/impl/drivers/pic.c \
        $(SRC_DIR)/impl/drivers/serial.c \
        $(SRC_DIR)/impl/kernel_mode/hal/hal.c \
        $(SRC_DIR)/impl/kernel_mode/hal/cpu/gdt.c \
        $(SRC_DIR)/impl/kernel_mode/hal/interrupts/idt.c \
        $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.c \
        $(SRC_DIR)/impl/kernel_mode/hal/io/ports.c \
        $(SRC_DIR)/impl/kernel/string.c

# Build artifacts
ASM_OBJ = $(BUILD_DIR)/$(ARCH)/boot.o \
          $(BUILD_DIR)/$(ARCH)/isr-asm.o \
          $(BUILD_DIR)/$(ARCH)/context_switch.o

C_OBJ = $(BUILD_DIR)/$(ARCH)/main.o \
        $(BUILD_DIR)/$(ARCH)/vga_graphics.o \
        $(BUILD_DIR)/$(ARCH)/font.o \
        $(BUILD_DIR)/$(ARCH)/cursor.o \
        $(BUILD_DIR)/$(ARCH)/keyboard.o \
        $(BUILD_DIR)/$(ARCH)/mouse.o \
        $(BUILD_DIR)/$(ARCH)/rtc.o \
        $(BUILD_DIR)/$(ARCH)/pic.o \
        $(BUILD_DIR)/$(ARCH)/serial.o \
        $(BUILD_DIR)/$(ARCH)/hal.o \
        $(BUILD_DIR)/$(ARCH)/gdt.o \
        $(BUILD_DIR)/$(ARCH)/hal_idt.o \
        $(BUILD_DIR)/$(ARCH)/hal_isr.o \
        $(BUILD_DIR)/$(ARCH)/hal_ports.o \
        $(BUILD_DIR)/$(ARCH)/string.o

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
INCLUDES = -I$(SRC_DIR)/intf

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
$(BUILD_DIR)/$(ARCH)/boot.o: $(SRC_DIR)/impl/x86_64/boot.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/$(ARCH)/isr-asm.o: $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/$(ARCH)/context_switch.o: $(SRC_DIR)/impl/kernel_mode/hal/cpu/context_switch.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

# Pattern rule for C files
$(BUILD_DIR)/$(ARCH)/%.o: $(SRC_DIR)/impl/%.c | $(BUILD_DIR)/$(ARCH)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Specific C file rules
$(BUILD_DIR)/$(ARCH)/main.o: $(SRC_DIR)/impl/kernel/main.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vga_graphics.o: $(SRC_DIR)/impl/graphics/vga_graphics.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/font.o: $(SRC_DIR)/impl/graphics/font.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/cursor.o: $(SRC_DIR)/impl/graphics/cursor.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/keyboard.o: $(SRC_DIR)/impl/drivers/keyboard.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mouse.o: $(SRC_DIR)/impl/drivers/mouse.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/rtc.o: $(SRC_DIR)/impl/drivers/rtc.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/pic.o: $(SRC_DIR)/impl/drivers/pic.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/serial.o: $(SRC_DIR)/impl/drivers/serial.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal.o: $(SRC_DIR)/impl/kernel_mode/hal/hal.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/gdt.o: $(SRC_DIR)/impl/kernel_mode/hal/cpu/gdt.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal_idt.o: $(SRC_DIR)/impl/kernel_mode/hal/interrupts/idt.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal_isr.o: $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal_ports.o: $(SRC_DIR)/impl/kernel_mode/hal/io/ports.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/string.o: $(SRC_DIR)/impl/kernel/string.c | $(BUILD_DIR)/$(ARCH)
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
	cp $(KERNEL_ELF) $(BUILD_DIR)/$(ARCH)/iso/boot/kernel.elf
	cp $(TARGETS_DIR)/$(ARCH)/grub.cfg $(BUILD_DIR)/$(ARCH)/iso/boot/grub/grub.cfg
	mkdir -p /tmp/gameros_iso && cp -r $(BUILD_DIR)/$(ARCH)/iso/* /tmp/gameros_iso/
	grub-mkrescue -o /tmp/kernel.iso /tmp/gameros_iso
	cp /tmp/kernel.iso $@
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
