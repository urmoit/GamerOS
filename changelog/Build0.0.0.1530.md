# Build 0.0.0.1530 Changelog

**Release Date:** 2025-11-11

## Overview

This build focuses on critical interrupt handling improvements and system stability fixes. Significant enhancements to ISR stack frame management, PIC interrupt masking, and controlled interrupt enabling have been implemented to address the persistent OS boot issues in QEMU. While the core BIOS interrupt problem remains unresolved, substantial progress has been made in interrupt infrastructure.

## Features

### Interrupt System Overhaul
- Complete rewrite of ISR handlers with proper 64-bit stack frame management
- All registers (RAX-R15) now properly saved and restored in interrupt context
- Correct `iretq` instruction usage for 64-bit interrupt returns
- Enhanced IRQ handlers with full register preservation

### PIC Interrupt Management
- Improved PIC initialization with proper masking during boot
- Controlled interrupt enabling after full system initialization
- Timer and keyboard interrupts selectively enabled while others remain masked
- Better EOI (End of Interrupt) handling for hardware interrupts

### Kernel Initialization Improvements
- Interrupts now enabled only after all critical initialization is complete
- Proper sequencing of hardware setup before interrupt activation
- Enhanced error handling in ISR routines (removed infinite halt loops)

## Technical Details

### Architecture
- Target: x86_64
- Bootloader: GRUB Multiboot2
- Kernel Format: ELF64
- Graphics: VGA graphics with custom font system
- Interrupt Model: Enhanced PIC-based interrupt handling with proper 64-bit stack frames

### Build Requirements
- Docker (for cross-compilation)
- QEMU (for testing and emulation)
- GCC cross-compiler for x86_64

## Changes from Previous Build (0.0.0.1512)

### Critical Bug Fixes
- **ISR Stack Frame Issues**: Complete rewrite of interrupt handlers with proper register saving/restoration
- **Improper CLI/STI Usage**: Interrupts now properly disabled/enabled in handlers
- **Premature STI**: Interrupts enabled only after full system initialization
- **PIC Interrupt Masking**: Hardware interrupts properly masked during boot process
- **Setup Screen Hang**: Removed infinite loop with timeout mechanism

### Interrupt System Improvements
- ISR handlers now save/restore all 64-bit registers (RAX-R15)
- Proper `iretq` instruction for 64-bit interrupt returns
- PIC interrupts masked during initialization, selectively enabled later
- Enhanced common_isr_handler with better error reporting

### Code Quality Enhancements
- Removed compiler warnings related to interrupt handling
- Better register preservation in assembly interrupt routines
- Improved interrupt handler documentation and comments

## Known Issues

- **Critical**: OS still enters paused state in QEMU due to BIOS interrupts triggering SMM activation
- BIOS interrupts occurring at lower level than normal interrupt handling
- System Management Mode (SMM) activation despite interrupt handling fixes
- Need to completely eliminate BIOS interrupt sources in long mode

## Files Changed

### New Files
- `build-iso.bat` - Dedicated script for building only the ISO image

### Modified Files
- `src/impl/x86_64/isr.asm` - Complete ISR handler rewrite with proper stack frames
- `src/impl/x86_64/isr.c` - Enhanced common interrupt handlers
- `src/impl/x86_64/idt.c` - Improved PIC initialization and masking
- `src/impl/kernel/main.c` - Controlled interrupt enabling after initialization
- `src/impl/x86_64/ui.c` - Fixed setup screen infinite loop
- `currentbugs.md` - Updated bug tracking and status

### Removed Files
- None

---

**Build Information:**
- Kernel Version: 1.1
- Build Number: 0.0.0.1530
- Architecture: x86_64
- Build System: Docker + Make
- Graphics: VGA with custom font system
- Interrupt System: Enhanced 64-bit PIC-based handling