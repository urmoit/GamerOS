# Build 0.0.0.1350 Changelog

**Release Date:** 2025-11-05

## Overview

This build introduces a new set of setup and utility scripts, simplifying the build, run, and clean processes. The project structure has been cleaned up by removing obsolete files and consolidating documentation.

## Features

### Build System
- ✅ New `build.bat` script to automate the entire build process.
- ✅ New `clean.bat` script to remove all build artifacts.
- ✅ New `run-qemu.bat` script to run the OS in QEMU.
- ✅ New `run-vmware.bat` script to run the OS in VMware.
- ✅ New `debug.bat` script to run the OS in QEMU with debug monitor.
- ✅ Enhanced error handling in all scripts to check for dependencies.

### Development Tools
- ✅ Simplified project root with fewer files.
- ✅ Updated `README.md` with clear instructions for the new scripts.
- ✅ Updated `.gitignore` to exclude the new scripts.

## Technical Details

### Architecture
- **Target:** x86_64
- **Bootloader:** GRUB Multiboot2
- **Kernel Format:** ELF64
- **Graphics:** VGA Text Mode (80x25)
- **Memory Model:** 64-bit long mode

### Build Requirements
- Docker (for cross-compilation)
- NASM (x86_64 assembler)
- GCC (cross-compiler)
- GRUB2 (bootloader)
- QEMU or VMware (for testing)

## Changes from Previous Build

### Build Infrastructure
- Replaced `run.bat` with a more comprehensive set of scripts (`build.bat`, `clean.bat`, `run-qemu.bat`, `run-vmware.bat`, `debug.bat`).
- Removed `run` and `debug` targets from `Makefile`.

### Documentation
- Consolidated all build and run instructions into `README.md`.
- Removed `BUILD.md`, `VMWARE-SETUP.md`, and `QUICKSTART.md`.

## Known Issues

- Kernel currently halts after initialization (no scheduler)
- No memory management yet
- No file system support
- No interrupt handling (except basic boot)
- Limited to text mode output

## Next Steps

- [ ] Implement proper memory manager
- [ ] Add interrupt descriptor table (IDT)
- [ ] Implement basic scheduler
- [ ] Add file system support
- [ ] Implement VGA graphics mode

## Files Changed

### New Files
- `build.bat`
- `clean.bat`
- `debug.bat`
- `run-qemu.bat`
- `changelog/Build0.0.0.1350.md`

### Modified Files
- `Makefile`
- `README.md`
- `.gitignore`
- `run-vmware.bat`

### Removed Files
- `run.bat`
- `BUILD.md`
- `VMWARE-SETUP.md`
- `QUICKSTART.md`

---

**Build Information:**
- Kernel Version: 1.0
- Build Number: 0.0.0.1350
- Architecture: x86_64
- Build System: Docker + Make
