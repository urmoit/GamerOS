# GamerOS

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/chosentechies/GamerOS)
[![License](https://img.shields.io/badge/license-Custom-red.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-x86__64-lightgrey.svg)](https://github.com/chosentechies/GamerOS)
[![Language](https://img.shields.io/badge/language-C%20%26%20Assembly-blue.svg)](https://github.com/chosentechies/GamerOS)
[![Architecture](https://img.shields.io/badge/architecture-64--bit-green.svg)](https://github.com/chosentechies/GamerOS)

GamerOS is a hobby `x86_64` operating system written in C and Assembly. It includes a custom kernel, graphics stack, desktop shell, input drivers, filesystem work, and built-in apps such as Settings, Explorer, Notepad, and About.

Build `1.500` is releasing April 3, 2026.

You can download the latest public ISO release from:

- [GamerOS Releases](https://github.com/urmoit/GamerOS/releases)

Current public release:

- `1.300`

Upcoming release:

- `1.400` - March 27, 2026
- `1.500` - April 3, 2026

> [!WARNING]
> **ACTIVE DEVELOPMENT**: This project is under heavy development. Features may change, break, or be incomplete. The name "GamerOS" is a placeholder and subject to change.

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

# 2. Build the OS locally from MSYS2/MinGW or WSL
make -C tools build-x86_64

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
| GCC | 7.0+ | C compiler |
| NASM | 2.13+ | Assembler |
| LD | 2.26+ | Linker |
| GRUB / xorriso | >= 2.06 | ISO image creation |
| [QEMU](https://www.qemu.org/) | Latest | Emulation and testing |

</details>

## Installation Guide

### Docker-free local builds

GamerOS now builds natively from a Unix-like shell on Windows (MSYS2, Git Bash, or WSL), or directly on a Linux host. You do **not** need Docker.

#### Method 1: MSYS2

Recommended for most Windows users because package installation is simpler.

```bash
# 1. Download and install MSYS2 from https://www.msys2.org/

# 2. Update MSYS2 (run twice)
pacman -Syu

# 3. Install toolchain
pacman -S mingw-w64-x86_64-gcc nasm mingw-w64-x86_64-binutils make

# 4. Add to PATH if needed
# C:\\msys64\\usr\\bin
# C:\\msys64\\mingw64\\bin
```

#### Method 2: WSL or Linux

From an Ubuntu/Debian-based shell:

```bash
sudo apt update
sudo apt install build-essential gcc-multilib nasm make grub-pc-bin xorriso binutils
```

> [!TIP]
> You can also use `x86_64-w64-mingw32-gcc` on Debian/Ubuntu to produce a Windows-targeted toolchain if desired.

#### Method 3: MinGW-w64 manual install (Windows)

<details>
<summary><b>Expand for manual setup steps</b></summary>

**GCC (MinGW-w64)**

1. Download from [winlibs.com](https://winlibs.com/)
2. Extract to `C:\\mingw64`
3. Add `C:\\mingw64\\bin` to `PATH`

**NASM**

1. Download from [nasm.us](https://www.nasm.us/pub/nasm/releasebuilds/)
2. Extract it
3. Add its binary folder to `PATH`

**LD / objcopy / grub-mkrescue / xorriso**

- `ld` and `objcopy` are included with MinGW-w64/binutils.
- For ISO creation on Windows, use the MSYS2 `grub-mkrescue` and `xorriso` packages (Method 1); on WSL/Linux, install `grub-pc-bin` and `xorriso` as shown above.

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

If you are new to QEMU, open the local setup guide:

- [QEMU setup tutorial](QemuTut.md)

### Verify Installation

Run these commands to confirm everything is set up:

```cmd
gcc --version
nasm -v
ld --version
objcopy --version
make --version
qemu-system-x86_64 --version
```

> [!IMPORTANT]
> Ensure your build shell provides `grub-mkrescue` and `xorriso` (MSYS2/WSL/Linux).

## Building and Running

```batch
Build from MSYS2/MinGW, Git Bash, WSL, or Linux:
make -C tools build-x86_64
```

```batch
REM Windows batch helpers:
build.bat         REM native Windows build entry point
build-iso.bat     REM rebuild ISO after existing build artifacts
run-qemu.bat      REM launch ISO in QEMU
debug.bat         REM launch QEMU with monitor attached
clean.bat         REM clean build outputs
```

Main ISO output:

```text
dist/x86_64/kernel.iso
```

### Advanced Options

<details>
<summary><b>Custom build configurations</b></summary>

```batch
REM Build from tools/ using native toolchain
make -C tools build-x86_64

REM Force a full rebuild
make -C tools clean build-x86_64

REM Use WSL Makefile layout
make -C tools -f Makefile.wsl build-x86_64
```

</details>

### Advanced Options

<details>
<summary><b>Custom build configurations</b></summary>

```batch
# Build specific target
make -C tools build-x86_64

# Verbose output
make -C tools build-x86_64 V=1

# Force a clean rebuild
make -C tools clean build-x86_64

# Use WSL Makefile layout
make -C tools -f Makefile.wsl build-x86_64
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
  tools/          Makefiles and local build entry points
  config/
    buildenv/     Legacy build environment
    targets/      Target linker scripts and GRUB config
  build/          Intermediate build output
  dist/           Final generated images
```

## Development Notes

- Use MSYS2, Git Bash, WSL, or Linux for local ISO builds via `make`.
- QEMU is usually the safer first test target.
- VMware support is still being stabilized.

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
5. Build and test with `make -C tools build-x86_64` before opening a pull request.
6. Build natively; batch helpers are supported for QEMU and cleanup but are not required for ISO builds.

## License

See [LICENSE](LICENSE).
