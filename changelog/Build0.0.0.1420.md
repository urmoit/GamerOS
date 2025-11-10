# Build 0.0.0.1420 Changelog

**Release Date:** 2025-11-10

## Overview

This build focuses on improving the development experience by updating QEMU configuration and adding educational resources for developers.

## Features

### Development Tools
- ✅ **Updated QEMU Configuration:** Modified `run-qemu.bat` and `debug.bat` to use specific QEMU executable path (`C:\Program Files\qemu\qemu-system-x86_64.exe`) instead of relying on PATH
- ✅ **QEMU Tutorial:** Added embedded YouTube tutorial video for QEMU setup and usage
- ✅ **Documentation Cleanup:** Removed VMware references from README.md to focus on QEMU as primary emulation platform

## Technical Details

### Architecture
- **Target:** x86_64
- **Bootloader:** GRUB Multiboot2
- **Kernel Format:** ELF64
- **Graphics:** VGA/VESA graphics with multiple modes and color depths
- **Memory Model:** 64-bit long mode

### Build Requirements
- Docker (for cross-compilation)
- QEMU (Windows executable at `C:\Program Files\qemu\qemu-system-x86_64.exe`)

## Changes from Previous Build (0.0.0.1400)

### Development Environment
- Updated QEMU batch scripts to use explicit executable path for better Windows compatibility
- Added QEMU tutorial resources for new developers
- Streamlined documentation by removing VMware-specific instructions

## Known Issues

- Kernel currently uses graphics mode but may fall back to text mode on some hardware
- Some video modes may not be supported on all virtualization platforms
- Memory management is basic bump allocator (no free functionality)

## Files Changed

### New Files
- `QemuTut.md` - QEMU tutorial with embedded video

### Modified Files
- `kernel.elf` - Rebuilt kernel binary
- `main.o` - Recompiled main kernel object
- `vga_graphics.o` - Recompiled graphics object
- `main.c` - Kernel main function updates
- `vga_graphics.c` - Graphics system improvements
- `graphics.h` - Graphics API enhancements
- `debug.bat` - Updated to use specific QEMU executable path
- `run-qemu.bat` - Updated to use specific QEMU executable path
- `build.bat` - Build script updates
- `README.md` - Removed VMware references, added QEMU tutorial

### Removed Files
- None

---

**Build Information:**
- Kernel Version: 1.0
- Build Number: 0.0.0.1420
- Architecture: x86_64
- Build System: Docker + Make
- Graphics: Enhanced VGA/VESA with multiple modes and color depths