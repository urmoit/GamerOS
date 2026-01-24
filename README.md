<div>
  <h1>GamerOS</h1>
  <p>
    A simple x86_64 hobby operating system with a Windows 11-like desktop environment, built from scratch.
    <br />
    <a href="#-quick-start"><strong>Getting Started »</strong></a>
    <br />
    <br />
    <a href="https://github.com/chosentechies/GamerOS/issues">Report Bug</a>
    ·
    <a href="https://github.com/chosentechies/GamerOS/issues">Request Feature</a>
  </p>
</div>

<div>
  <img src="https://img.shields.io/badge/build-passing-brightgreen.svg" alt="Build Status">
  <img src="https://img.shields.io/badge/license-Custom-red.svg" alt="License">
  <img src="https://img.shields.io/badge/platform-x86__64-lightgrey.svg" alt="Platform">
  <br />
  <img src="https://img.shields.io/badge/language-C%20%26%20Assembly-blue.svg" alt="Languages">
  <img src="https://img.shields.io/badge/architecture-64--bit-green.svg" alt="Architecture">

 
  [![Repobeats analytics image](https://repobeats.axiom.co/api/embed/7a3672ae3c424e1c04e25afe6ef553652e788130.svg "Repobeats analytics image")](https://github.com/urmoit/GamerOS/pulse)


> ⚠️ **WORK IN PROGRESS** ⚠️
>
> This operating system project is under heavy development. Features may change, break, or be incomplete at any time. The name "GamerOS" is a placeholder.
>

## 📋 Table of Contents

- [🚀 Quick Start](#-quick-start)
- [📋 Prerequisites](#-prerequisites)
- [🛠️ Installation Guide](#️-installation-guide)
- [🏗️ Building and Running](#️-building-and-running)
- [📖 About The Project](#-about-the-project)
- [🐛 Bug Tracking](#-bug-tracking)
- [📁 Project Structure](#-project-structure)
- [🤝 Contributing](#-contributing)
- [📚 Learning Resources](#-learning-resources)
- [📄 License](#-license)
- [🙏 Credits](#-credits)

## 🚀 Quick Start

Get GamerOS running in 5 minutes:

1. **Install Prerequisites**: Docker, GCC, NASM, QEMU
2. **Clone & Build**: `git clone <repo> && cd GamerOS && build.bat`
3. **Run**: `run-qemu.bat`
4. **Enjoy**: Your custom OS boots with a Windows 11-style desktop!

> 💡 **Tip**: Follow the detailed [Installation Guide](#️-installation-guide) for step-by-step instructions.

## 📋 Prerequisites

### System Requirements

- **Operating System**: Windows 10/11 (64-bit)
- **RAM**: 4GB minimum (8GB recommended)
- **Disk Space**: 2GB free space
- **Internet**: Required for downloading tools

### Required Software

- **[Docker](https://www.docker.com/get-started)** - For cross-compilation environment
- **GCC** (7.0+) - C compiler
- **NASM** (2.13+) - Assembler
- **LD** (2.26+) - Linker
- **[QEMU](https://www.qemu.org/)** - Emulator for testing

## 🛠️ Installation Guide

This comprehensive guide covers installing all required tools. Choose between direct downloads or MSYS2 package manager.

### Step 1: Install Docker

Docker is essential for building GamerOS.

1. Download [Docker Desktop](https://www.docker.com/products/docker-desktop/)
2. Install and run Docker Desktop
3. Verify: `docker --version`

### Step 2: Choose Your Toolchain Installation Method

#### Option A: MSYS2 (Recommended for Beginners)

MSYS2 provides easy package management.

**Download & Install MSYS2:**
- Get the installer from [msys2.org](https://www.msys2.org/)
- Run installer (default settings)
- Open MSYS2 MSYS terminal
- Update: `pacman -Syu` (run twice if needed)
- Add to PATH: `C:\msys64\usr\bin` and `C:\msys64\mingw64\bin`

**Install Tools via MSYS2:**
```bash
pacman -S mingw-w64-x86_64-gcc nasm mingw-w64-x86_64-binutils make
```

> 📝 **Note**: This installs GCC, NASM, LD, and Make, but **does not install QEMU**. QEMU requires separate installation as described in Step 3.

#### Option B: Direct Downloads

For manual installation:

**GCC (MinGW-w64):**
- Download from [winlibs.com](https://winlibs.com/)
- Extract to `C:\mingw64`
- Add `C:\mingw64\bin` to PATH

**NASM:**
- Download from [nasm.us](https://www.nasm.us/pub/nasm/releasebuilds/)
- Extract and add to PATH

**LD (Binutils):**
- Included with GCC above

### Step 3: Install QEMU

**Download:** [QEMU Installer](https://qemu.weilnetz.de/w64/)
- Run installer
- Ensure QEMU is in PATH

> 📹 **Video Tutorial**: [Complete QEMU Setup Guide](https://www.youtube.com/watch?v=HywXtRz0URE) - Step-by-step video walkthrough

### Step 4: Verify Installation

Open Command Prompt and run:
```cmd
gcc --version
nasm -v
ld --version
qemu-system-x86_64 --version
docker --version
```

All commands should show version information.

> 🔧 **Troubleshooting**: If commands aren't found, check your PATH environment variable.

## 🏗️ Building and Running

### Build the OS

```batch
# Clone repository
git clone https://github.com/chosentechies/GamerOS.git
cd GamerOS

# Build (requires Docker)
build.bat
```

### Run in QEMU

```batch
run-qemu.bat
```

### Debug Mode

```batch
debug.bat
```

### Clean Build

```batch
clean.bat
```

## 📖 About The Project

GamerOS is a 64-bit hobby operating system written from scratch in C and x86-64 Assembly. It features a modern Windows 11-inspired desktop environment with direct hardware access.

**Key Technologies:**
- Custom 64-bit kernel
- Hardware abstraction layer (HAL)
- Graphical user interface (GUI)
- Process and memory management
- File system support

## 🐛 Bug Tracking

- **[Current Bugs & Issues](currentbugs.md)** - Comprehensive tracking of all known problems
- **Status**: 71% of issues resolved
- **Critical Issues**: 0 remaining
- **Logic Errors**: 0 remaining

## 📁 Project Structure

```
GamerOS/
├── src/                          # Source code
│   ├── intf/                    # Interfaces and headers
│   └── impl/                     # Implementations
│       ├── drivers/              # Hardware drivers
│       ├── filesystem/           # File system
│       ├── graphics/             # Graphics system
│       ├── kernel/               # Kernel components
│       ├── kernel_mode/          # Kernel mode
│       │   ├── hal/              # Hardware abstraction layer
│       │   │   ├── cpu/
│       │   │   ├── interrupts/
│       │   │   ├── io/
│       │   │   └── memory/
│       │   └── microkernel/      # Microkernel
│       ├── ui_system/            # UI system
│       └── x86_64/               # x86-64 specific
├── executive/                    # Executive layer
│   ├── filesystem_manager/
│   ├── gdi/
│   ├── io_manager/
│   │   └── drivers/
│   ├── ipc_manager/
│   ├── object_manager/
│   ├── power_manager/
│   ├── process_manager/
│   └── security_manager/
├── user_mode/                    # User mode
│   ├── compatibility_layers/     # Compatibility layers
│   │   ├── msdos/
│   │   └── windows9x/
│   ├── environment_subsystems/   # Environment subsystems
│   │   ├── os2/
│   │   ├── posix/
│   │   └── win32/
│   ├── integral_subsystems/      # Integral subsystems
│   │   ├── security/
│   │   ├── server_service/
│   │   └── workstation/          # Workstation
│   └── interfaces/               # Interfaces
├── build/                        # Build artifacts
│   └── x86_64/                   # x86-64 build files
├── buildenv/                     # Docker build environment
├── targets/                      # Build targets
│   └── x86_64/
└── *.bat                         # Build scripts
└── *.md                          # Documentation
└── *.log                         # Logs
```

## 🤝 Contributing

We welcome contributions! Here's how to get involved:

### Ways to Contribute
- 🐛 **Report Bugs**: Use [GitHub Issues](https://github.com/chosentechies/GamerOS/issues)
- 💡 **Suggest Features**: Open a feature request
- 🔧 **Code Contributions**: Fork and submit pull requests
- 📖 **Documentation**: Improve guides and docs

### Development Setup
1. Follow the [Installation Guide](#️-installation-guide)
2. Read the [Architecture Documentation](executive_layer_design.md)
3. Check [Current Bugs](currentbugs.md) for known issues
4. Submit PRs with clear descriptions

### Guidelines
- Follow existing code style
- Add tests for new features
- Update documentation
- Ensure builds pass

## 📚 Learning Resources

### Official Documentation
- **OSDev Wiki**: [Comprehensive OS development guide](https://wiki.osdev.org/)
- **x86-64 Architecture**: Intel/AMD manuals
- **C Programming**: Standard library references

### Video Tutorials
- **QEMU Setup**: [Complete QEMU Tutorial](https://www.youtube.com/watch?v=HywXtRz0URE)
- **OS Development**: Search YouTube for "x86 OS development"

### Books & Articles
- "Operating System Concepts" by Silberschatz
- "Computer Systems: A Programmer's Perspective"
- Online articles on kernel development

## 📄 License

This project is licensed under a custom license. See [LICENSE](LICENSE) for more information.

## 🙏 Credits

**Author:** Chosentechies

**Inspiration:** Modern desktop operating systems

**Special Thanks:**
- OSDev community
- Free software projects (GCC, NASM, QEMU)
- Open source contributors

---

<div>
  <p>Made with ❤️ for learning and fun</p>
  <p>
    <a href="#-table-of-contents">Back to Top</a>
  </p>
</div>
