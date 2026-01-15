ARCH ?= x86_64

# Directories
SRC_DIR = src
BUILD_DIR = build
DIST_DIR = dist
TARGETS_DIR = targets

# Source files
ASM_SRC = $(SRC_DIR)/impl/x86_64/boot.asm \
        $(SRC_DIR)/impl/kernel_mode/hal/cpu/context_switch.asm \
        $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.asm
C_SRC = $(SRC_DIR)/impl/kernel/main.c \
        $(SRC_DIR)/impl/graphics/vga_graphics.c \
        $(SRC_DIR)/impl/graphics/font.c \
        $(SRC_DIR)/impl/ui_system/ui.c \
        $(SRC_DIR)/impl/ui_system/ui_widgets.c \
        $(SRC_DIR)/impl/drivers/rtc.c \
        $(SRC_DIR)/impl/ui_system/window.c \
        $(SRC_DIR)/impl/kernel/string.c \
        $(SRC_DIR)/impl/drivers/keyboard.c \
        $(SRC_DIR)/impl/drivers/pic.c \
        $(SRC_DIR)/impl/drivers/mouse.c \
        $(SRC_DIR)/impl/gui_app.c \
        $(SRC_DIR)/impl/kernel_mode/hal/hal.c \
        $(SRC_DIR)/impl/kernel_mode/hal/cpu/gdt.c \
        $(SRC_DIR)/impl/kernel_mode/hal/interrupts/idt.c \
        $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.c \
        $(SRC_DIR)/impl/kernel_mode/hal/io/ports.c \
        $(SRC_DIR)/impl/kernel_mode/hal/memory/paging.c \
        $(SRC_DIR)/impl/kernel_mode/microkernel/ipc.c \
        $(SRC_DIR)/impl/kernel_mode/microkernel/memory.c \
        $(SRC_DIR)/impl/kernel_mode/microkernel/process.c \
        $(SRC_DIR)/executive/executive.c \
        $(SRC_DIR)/executive/object_manager/object_manager.c \
        $(SRC_DIR)/executive/filesystem_manager/filesystem_manager.c \
        $(SRC_DIR)/executive/io_manager/io_manager.c \
        $(SRC_DIR)/user_mode/user_mode.c \
        $(SRC_DIR)/user_mode/integral_subsystems/workstation/workstation.c \
        $(SRC_DIR)/user_mode/integral_subsystems/workstation/desktop_manager.c \
        $(SRC_DIR)/user_mode/integral_subsystems/workstation/ui_framework.c \
        $(SRC_DIR)/user_mode/integral_subsystems/workstation/widget_system.c \
        $(SRC_DIR)/user_mode/integral_subsystems/workstation/window_manager.c \
        $(SRC_DIR)/user_mode/integral_subsystems/security/security.c \
        $(SRC_DIR)/user_mode/integral_subsystems/server_service/server_service.c \
        $(SRC_DIR)/user_mode/environment_subsystems/win32/win32.c \
        $(SRC_DIR)/user_mode/environment_subsystems/posix/posix.c \
        $(SRC_DIR)/user_mode/environment_subsystems/os2/os2.c \
        $(SRC_DIR)/user_mode/compatibility_layers/windows9x/windows9x.c \
        $(SRC_DIR)/user_mode/compatibility_layers/msdos/msdos.c \

# Build artifacts
ASM_OBJ = $(BUILD_DIR)/$(ARCH)/boot.o \
        $(BUILD_DIR)/$(ARCH)/context_switch.o \
        $(BUILD_DIR)/$(ARCH)/isr-asm.o
C_OBJ = $(BUILD_DIR)/$(ARCH)/main.o \
        $(BUILD_DIR)/$(ARCH)/vga_graphics.o \
        $(BUILD_DIR)/$(ARCH)/font.o \
        $(BUILD_DIR)/$(ARCH)/ui.o \
        $(BUILD_DIR)/$(ARCH)/ui_widgets.o \
        $(BUILD_DIR)/$(ARCH)/rtc.o \
        $(BUILD_DIR)/$(ARCH)/window.o \
        $(BUILD_DIR)/$(ARCH)/string.o \
        $(BUILD_DIR)/$(ARCH)/keyboard.o \
        $(BUILD_DIR)/$(ARCH)/pic.o \
        $(BUILD_DIR)/$(ARCH)/mouse.o \
        $(BUILD_DIR)/$(ARCH)/gui_app.o \
        $(BUILD_DIR)/$(ARCH)/hal.o \
        $(BUILD_DIR)/$(ARCH)/gdt.o \
        $(BUILD_DIR)/$(ARCH)/hal_idt.o \
        $(BUILD_DIR)/$(ARCH)/hal_isr.o \
        $(BUILD_DIR)/$(ARCH)/hal_ports.o \
        $(BUILD_DIR)/$(ARCH)/paging.o \
        $(BUILD_DIR)/$(ARCH)/microkernel_ipc.o \
        $(BUILD_DIR)/$(ARCH)/microkernel_memory.o \
        $(BUILD_DIR)/$(ARCH)/microkernel_process.o \
        $(BUILD_DIR)/$(ARCH)/executive.o \
        $(BUILD_DIR)/$(ARCH)/object_manager.o \
        $(BUILD_DIR)/$(ARCH)/filesystem_manager.o \
        $(BUILD_DIR)/$(ARCH)/io_manager.o \
        $(BUILD_DIR)/$(ARCH)/user_mode.o \
        $(BUILD_DIR)/$(ARCH)/workstation.o \
        $(BUILD_DIR)/$(ARCH)/desktop_manager.o \
        $(BUILD_DIR)/$(ARCH)/ui_framework.o \
        $(BUILD_DIR)/$(ARCH)/widget_system.o \
        $(BUILD_DIR)/$(ARCH)/window_manager.o \
        $(BUILD_DIR)/$(ARCH)/security.o \
        $(BUILD_DIR)/$(ARCH)/server_service.o \
        $(BUILD_DIR)/$(ARCH)/win32.o \
        $(BUILD_DIR)/$(ARCH)/posix.o \
        $(BUILD_DIR)/$(ARCH)/os2.o \
        $(BUILD_DIR)/$(ARCH)/windows9x.o \
        $(BUILD_DIR)/$(ARCH)/msdos.o \
OBJS = $(ASM_OBJ) $(C_OBJ)

# Output files
KERNEL_BIN = $(DIST_DIR)/$(ARCH)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/$(ARCH)/kernel.elf
ISO = $(DIST_DIR)/$(ARCH)/kernel.iso

# Tool version checking
REQUIRED_GCC_VERSION = 7.0
REQUIRED_NASM_VERSION = 2.13
REQUIRED_LD_VERSION = 2.26

# Check tool versions
.PHONY: check-tools
check-tools:
	@echo "Checking build tool versions..."
	@if ! gcc --version | head -n1 | grep -q " $(REQUIRED_GCC_VERSION)"; then \
		echo "Warning: GCC version $(REQUIRED_GCC_VERSION)+ recommended"; \
	fi
	@if ! nasm -v | grep -q "$(REQUIRED_NASM_VERSION)"; then \
		echo "Warning: NASM version $(REQUIRED_NASM_VERSION)+ recommended"; \
	fi
	@if ! ld --version | head -n1 | grep -q " $(REQUIRED_LD_VERSION)"; then \
		echo "Warning: LD version $(REQUIRED_LD_VERSION)+ recommended"; \
	fi

# Compiler flags
ASM = nasm
ASMFLAGS = -f elf64

CC = gcc
CFLAGS = -m64 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -std=c11 -Wno-unused-parameter -Wno-unused-variable -g
INCLUDES = -I$(SRC_DIR)/intf

LD = ld
LDFLAGS = -m elf_x86_64 -T $(TARGETS_DIR)/$(ARCH)/linker.ld -nostdlib

# Default target
.PHONY: all
all: check-tools build-$(ARCH)

# Build kernel for x86_64
.PHONY: build-x86_64
build-x86_64: $(ISO)

# Create directories
$(BUILD_DIR)/$(ARCH):
	mkdir -p $(BUILD_DIR)/$(ARCH)

$(DIST_DIR)/$(ARCH):
	mkdir -p $(DIST_DIR)/$(ARCH)

# Compile assembly with individual rules to avoid conflicts
$(BUILD_DIR)/$(ARCH)/boot.o: $(SRC_DIR)/impl/x86_64/boot.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

$(BUILD_DIR)/$(ARCH)/context_switch.o: $(SRC_DIR)/impl/kernel_mode/hal/cpu/context_switch.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

# Compile C with dependency tracking
$(BUILD_DIR)/$(ARCH)/main.o: $(SRC_DIR)/impl/kernel/main.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/vga_graphics.o: $(SRC_DIR)/impl/graphics/vga_graphics.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/font.o: $(SRC_DIR)/impl/graphics/font.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ui.o: $(SRC_DIR)/impl/ui_system/ui.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ui_widgets.o: $(SRC_DIR)/impl/ui_system/ui_widgets.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/rtc.o: $(SRC_DIR)/impl/drivers/rtc.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/window.o: $(SRC_DIR)/impl/ui_system/window.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/string.o: $(SRC_DIR)/impl/kernel/string.c | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<



$(BUILD_DIR)/$(ARCH)/keyboard.o: $(SRC_DIR)/impl/drivers/keyboard.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/pic.o: $(SRC_DIR)/impl/drivers/pic.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/mouse.o: $(SRC_DIR)/impl/drivers/mouse.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


$(BUILD_DIR)/$(ARCH)/gui_app.o: $(SRC_DIR)/impl/gui_app.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/executive.o: $(SRC_DIR)/executive/executive.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/object_manager.o: $(SRC_DIR)/executive/object_manager/object_manager.c $(SRC_DIR)/executive/object_manager/object_manager.h $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/filesystem_manager.o: $(SRC_DIR)/executive/filesystem_manager/filesystem_manager.c $(SRC_DIR)/executive/filesystem_manager/filesystem_manager.h $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/io_manager.o: $(SRC_DIR)/executive/io_manager/io_manager.c $(SRC_DIR)/executive/io_manager/io_manager.h $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/user_mode.o: $(SRC_DIR)/user_mode/user_mode.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<




# Kernel mode HAL compilation rules
$(BUILD_DIR)/$(ARCH)/gdt.o: $(SRC_DIR)/impl/kernel_mode/hal/cpu/gdt.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal_idt.o: $(SRC_DIR)/impl/kernel_mode/hal/interrupts/idt.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal_isr.o: $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal.o: $(SRC_DIR)/impl/kernel_mode/hal/hal.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/hal_ports.o: $(SRC_DIR)/impl/kernel_mode/hal/io/ports.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/paging.o: $(SRC_DIR)/impl/kernel_mode/hal/memory/paging.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Microkernel compilation rules
$(BUILD_DIR)/$(ARCH)/microkernel_ipc.o: $(SRC_DIR)/impl/kernel_mode/microkernel/ipc.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/microkernel_memory.o: $(SRC_DIR)/impl/kernel_mode/microkernel/memory.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/microkernel_process.o: $(SRC_DIR)/impl/kernel_mode/microkernel/process.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/isr-asm.o: $(SRC_DIR)/impl/kernel_mode/hal/interrupts/isr.asm | $(BUILD_DIR)/$(ARCH)
	$(ASM) $(ASMFLAGS) -o $@ $<

# User mode compilation rules
$(BUILD_DIR)/$(ARCH)/workstation.o: $(SRC_DIR)/user_mode/integral_subsystems/workstation/workstation.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/desktop_manager.o: $(SRC_DIR)/user_mode/integral_subsystems/workstation/desktop_manager.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/ui_framework.o: $(SRC_DIR)/user_mode/integral_subsystems/workstation/ui_framework.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/widget_system.o: $(SRC_DIR)/user_mode/integral_subsystems/workstation/widget_system.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/window_manager.o: $(SRC_DIR)/user_mode/integral_subsystems/workstation/window_manager.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/security.o: $(SRC_DIR)/user_mode/integral_subsystems/security/security.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/server_service.o: $(SRC_DIR)/user_mode/integral_subsystems/server_service/server_service.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/win32.o: $(SRC_DIR)/user_mode/environment_subsystems/win32/win32.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/posix.o: $(SRC_DIR)/user_mode/environment_subsystems/posix/posix.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/os2.o: $(SRC_DIR)/user_mode/environment_subsystems/os2/os2.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/windows9x.o: $(SRC_DIR)/user_mode/compatibility_layers/windows9x/windows9x.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BUILD_DIR)/$(ARCH)/msdos.o: $(SRC_DIR)/user_mode/compatibility_layers/msdos/msdos.c $(SRC_DIR)/intf/*.h | $(BUILD_DIR)/$(ARCH)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Link kernel
$(KERNEL_ELF): $(BUILD_DIR)/$(ARCH)/boot.o \
               $(BUILD_DIR)/$(ARCH)/context_switch.o \
               $(BUILD_DIR)/$(ARCH)/isr-asm.o \
               $(BUILD_DIR)/$(ARCH)/main.o \
               $(BUILD_DIR)/$(ARCH)/vga_graphics.o \
               $(BUILD_DIR)/$(ARCH)/font.o \
               $(BUILD_DIR)/$(ARCH)/ui.o \
               $(BUILD_DIR)/$(ARCH)/ui_widgets.o \
               $(BUILD_DIR)/$(ARCH)/rtc.o \
               $(BUILD_DIR)/$(ARCH)/window.o \
               $(BUILD_DIR)/$(ARCH)/string.o \
               $(BUILD_DIR)/$(ARCH)/keyboard.o \
               $(BUILD_DIR)/$(ARCH)/pic.o \
               $(BUILD_DIR)/$(ARCH)/mouse.o \
               $(BUILD_DIR)/$(ARCH)/gui_app.o \
               $(BUILD_DIR)/$(ARCH)/hal.o \
               $(BUILD_DIR)/$(ARCH)/gdt.o \
               $(BUILD_DIR)/$(ARCH)/hal_idt.o \
               $(BUILD_DIR)/$(ARCH)/hal_isr.o \
               $(BUILD_DIR)/$(ARCH)/hal_ports.o \
               $(BUILD_DIR)/$(ARCH)/paging.o \
               $(BUILD_DIR)/$(ARCH)/microkernel_ipc.o \
               $(BUILD_DIR)/$(ARCH)/microkernel_memory.o \
               $(BUILD_DIR)/$(ARCH)/microkernel_process.o \
               $(BUILD_DIR)/$(ARCH)/executive.o \
               $(BUILD_DIR)/$(ARCH)/object_manager.o \
               $(BUILD_DIR)/$(ARCH)/filesystem_manager.o \
               $(BUILD_DIR)/$(ARCH)/io_manager.o \
               $(BUILD_DIR)/$(ARCH)/user_mode.o \
               $(BUILD_DIR)/$(ARCH)/workstation.o \
               $(BUILD_DIR)/$(ARCH)/desktop_manager.o \
               $(BUILD_DIR)/$(ARCH)/ui_framework.o \
               $(BUILD_DIR)/$(ARCH)/widget_system.o \
               $(BUILD_DIR)/$(ARCH)/window_manager.o \
               $(BUILD_DIR)/$(ARCH)/security.o \
               $(BUILD_DIR)/$(ARCH)/server_service.o \
               $(BUILD_DIR)/$(ARCH)/win32.o \
               $(BUILD_DIR)/$(ARCH)/posix.o \
               $(BUILD_DIR)/$(ARCH)/os2.o \
               $(BUILD_DIR)/$(ARCH)/windows9x.o \
               $(BUILD_DIR)/$(ARCH)/msdos.o
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

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)/$(ARCH)/kernel.bin $(DIST_DIR)/$(ARCH)/kernel.elf 2>/dev/null || true
	rm -f $(DIST_DIR)/$(ARCH)/kernel.iso 2>/dev/null || true



