# GamerOS

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/chosentechies/GamerOS)
[![License](https://img.shields.io/badge/license-Custom-red.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-x86__64-lightgrey.svg)](https://github.com/chosentechies/GamerOS)
[![Language](https://img.shields.io/badge/language-C%20%26%20Assembly-blue.svg)](https://github.com/chosentechies/GamerOS)
[![Architecture](https://img.shields.io/badge/architecture-64--bit-green.svg)](https://github.com/chosentechies/GamerOS)

GamerOS is a hobby `x86_64` operating system written in C and Assembly. It includes a custom kernel, graphics stack, desktop shell, input drivers, filesystem work, and built-in apps such as Settings, Explorer, Notepad, and About.

Build `1.400` is coming soon.

You can download the latest public ISO release from:

- [GamerOS Releases](https://github.com/urmoit/GamerOS/releases)

Current public release:

- `1.300`

Upcoming release:

- `1.400` coming soon

> [!WARNING]
> **ACTIVE DEVELOPMENT**: This project is under heavy development. Features may change, break, or be incomplete. The name "GamerOS" is a placeholder and subject to change.

> [!NOTE]
> **Progress Update**: 71% of tracked issues resolved • 0 critical bugs • 0 logic errors

## Overview

Current development focuses on:

- framebuffer and VGA graphics paths
- desktop shell and windowing behavior
- built-in app launching through the `GOSAPP` loader format
- filesystem and storage layout bootstrapping
- display settings and runtime resolution switching
- VMware boot and app-launch stability fixes

## Quick Start

Get GamerOS running in under 5 minutes:

```batch
# 1. Clone the repository
git clone https://github.com/chosentechies/GamerOS.git
cd GamerOS

# 2. Build the OS (requires Docker)
build.bat

# 3. Launch in QEMU
run-qemu.bat
```

First time? Follow the [detailed installation guide](#installation-guide) below.

## Prerequisites

<details>
<summary><b>System Requirements</b></summary>

- **OS**: Windows 10/11 (64-bit)
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 2GB free space
- **Network**: Required for downloading dependencies

</details>

<details>
<summary><b>Required Software</b></summary>

| Tool | Version | Purpose |
|------|---------|---------|
| [Docker Desktop](https://www.docker.com/get-started) | Latest | Cross-compilation environment |
| GCC | 7.0+ | C compiler |
| NASM | 2.13+ | Assembler |
| LD | 2.26+ | Linker |
| [QEMU](https://www.qemu.org/) | Latest | Emulation and testing |

</details>

## Installation Guide

### Method 1: MSYS2

Recommended for most users because package installation is simpler.

```bash
# 1. Download and install MSYS2 from https://www.msys2.org/

# 2. Update MSYS2 (run twice)
pacman -Syu

# 3. Install toolchain
pacman -S mingw-w64-x86_64-gcc nasm mingw-w64-x86_64-binutils make

# 4. Add to PATH
# C:\msys64\usr\bin
# C:\msys64\mingw64\bin
```

### Method 2: Manual Installation

<details>
<summary><b>Expand for manual setup steps</b></summary>

**GCC (MinGW-w64)**

1. Download from [winlibs.com](https://winlibs.com/)
2. Extract to `C:\mingw64`
3. Add `C:\mingw64\bin` to `PATH`

**NASM**

1. Download from [nasm.us](https://www.nasm.us/pub/nasm/releasebuilds/)
2. Extract it
3. Add its binary folder to `PATH`

**LD**

Included with GCC / MinGW-w64.

</details>

### Installing QEMU

> [!IMPORTANT]
> QEMU must be installed separately regardless of which method you choose above.

1. Download the [QEMU Windows installer](https://qemu.weilnetz.de/w64/)
2. Run the installer with default settings
3. Verify the install:

```cmd
qemu-system-x86_64 --version
```

If you are new to QEMU, this setup tutorial may help:

- [QEMU setup tutorial](https://www.youtube.com/watch?v=HywXtRz0URE)

### Installing Docker

1. Download [Docker Desktop](https://www.docker.com/products/docker-desktop/)
2. Install and launch Docker Desktop
3. Verify the install:

```cmd
docker --version
```

### Verify Installation

Run these commands to confirm everything is set up:

```cmd
gcc --version
nasm -v
ld --version
qemu-system-x86_64 --version
docker --version
```

> [!TIP]
> If any command fails, check that the tool's install directory is present in your `PATH`.

## Building and Running

### Basic Commands

```batch
# Build the OS image
build.bat

# Run in QEMU emulator
run-qemu.bat

# Launch with debugging
debug.bat

# Clean build artifacts
clean.bat
```

Main ISO output:

```text
dist/x86_64/kernel.iso
```

### Advanced Options

<details>
<summary><b>Custom build configurations</b></summary>

```batch
# Build specific target
build.bat x86_64

# Verbose output
build.bat --verbose

# Skip Docker rebuild
build.bat --no-rebuild
```

</details>

## Project Layout

```text
GamerOS/
  src/
    apps/         Built-in app UI and app-facing logic
    impl/         Kernel, drivers, graphics, filesystem, architecture code
    intf/         Shared interfaces and headers
    resources/    Embedded assets
    user_mode/    Experimental user-mode and subsystem code
  targets/        Linker scripts and target boot assets
  buildenv/       Docker build environment
  build/          Intermediate build output
  dist/           Final generated images
```

## Development Notes

- QEMU is usually the safer first test target.
- VMware support is still being stabilized.
- Repeated Docker ISO builds are supported in the current packaging flow.
- There are still known non-fatal NASM warnings from `src/impl/x86_64/boot.asm` related to `.bss` initialization.

## Important Files

- [changelog_2026-xx-xx.md](changelog_2026-xx-xx.md)
- [walkthrough_src_bugfixes_2026-xx-xx.md](walkthrough_src_bugfixes_2026-xx-xx.md)
- [currentbugs.md](currentbugs.md)
- [executive_layer_design.md](executive_layer_design.md)

## Contributing

1. Fork the repository.
2. Create a branch for your work.
3. Keep changes focused.
4. Update documentation when behavior changes.
5. Test in at least one VM or emulator before opening a pull request.

## License

See [LICENSE](LICENSE).
