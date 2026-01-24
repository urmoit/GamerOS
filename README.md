# GamerOS

<div align="center">

**A modern x86_64 hobby operating system with a Windows 11-inspired desktop environment**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/chosentechies/GamerOS)
[![License](https://img.shields.io/badge/license-Custom-red.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-x86__64-lightgrey.svg)](https://github.com/chosentechies/GamerOS)
[![Language](https://img.shields.io/badge/language-C%20%26%20Assembly-blue.svg)](https://github.com/chosentechies/GamerOS)
[![Architecture](https://img.shields.io/badge/architecture-64--bit-green.svg)](https://github.com/chosentechies/GamerOS)

[Getting Started](#-quick-start) • [Documentation](#-project-structure) • [Contributing](#-contributing) • [Report Bug](https://github.com/chosentechies/GamerOS/issues)

![Repobeats analytics](https://repobeats.axiom.co/api/embed/1d66b34dad5bb8f95e429c7d37c1559c50b08608.svg)

</div>

---

> [!WARNING]
> **ACTIVE DEVELOPMENT**: This project is under heavy development. Features may change, break, or be incomplete. The name "GamerOS" is a placeholder and subject to change.

> [!NOTE]
> **Progress Update**: 71% of tracked issues resolved • 0 critical bugs • 0 logic errors

## ✨ Features

<table>
<tr>
<td width="50%">

### Core System
- 🔧 Custom 64-bit microkernel architecture
- 💾 Advanced memory management with paging
- ⚡ Hardware abstraction layer (HAL)
- 🔄 Process and thread scheduling
- 📁 Native file system support

</td>
<td width="50%">

### User Experience
- 🎨 Windows 11-inspired desktop environment
- 🖼️ Hardware-accelerated graphics
- 🖱️ Native input device support
- 🔌 Modular driver architecture
- 🛡️ Security and process isolation

</td>
</tr>
</table>

## 🚀 Quick Start

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

**First time?** Follow the [detailed installation guide](#%EF%B8%8F-installation-guide) below.

## 📋 Prerequisites

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

## 🛠️ Installation Guide

### Method 1: MSYS2 (Recommended)

**Best for beginners** - Automated package management

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
3. Add `C:\mingw64\bin` to PATH

**NASM**
1. Download from [nasm.us](https://www.nasm.us/pub/nasm/releasebuilds/)
2. Extract and add to PATH

**LD** - Included with GCC

</details>

### Installing QEMU

> [!IMPORTANT]
> QEMU must be installed separately regardless of which method you choose above.

1. Download the [QEMU Windows installer](https://qemu.weilnetz.de/w64/)
2. Run the installer with default settings
3. Verify installation: `qemu-system-x86_64 --version`

📹 **New to QEMU?** Watch this [complete setup tutorial](https://www.youtube.com/watch?v=HywXtRz0URE)

### Installing Docker

1. Download [Docker Desktop](https://www.docker.com/products/docker-desktop/)
2. Install and launch Docker Desktop
3. Verify: `docker --version`

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
> If any command fails, check that the tool's directory is in your PATH environment variable.

## 🏗️ Building and Running

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

## 📁 Project Architecture

```
GamerOS/
├── 🔧 src/                    # Source code
│   ├── intf/                  # Public interfaces
│   └── impl/                  # Implementations
│       ├── drivers/           # Hardware drivers
│       ├── filesystem/        # File system layer
│       ├── graphics/          # Graphics subsystem
│       ├── kernel/            # Core kernel
│       ├── kernel_mode/       # Kernel mode components
│       │   ├── hal/           # Hardware abstraction
│       │   └── microkernel/   # Microkernel core
│       ├── ui_system/         # User interface
│       └── x86_64/            # Architecture-specific
│
├── ⚙️ executive/              # Executive services
│   ├── filesystem_manager/
│   ├── gdi/                   # Graphics device interface
│   ├── io_manager/
│   ├── ipc_manager/           # Inter-process communication
│   ├── object_manager/
│   ├── power_manager/
│   ├── process_manager/
│   └── security_manager/
│
├── 👤 user_mode/              # User space
│   ├── compatibility_layers/
│   ├── environment_subsystems/
│   └── integral_subsystems/
│
├── 📦 build/                  # Build artifacts
├── 🐋 buildenv/               # Docker environment
└── 🎯 targets/                # Target configurations
```

<details>
<summary><b>View detailed architecture</b></summary>

For comprehensive design documentation, see [executive_layer_design.md](executive_layer_design.md)

**Key Components:**
- **HAL**: Hardware abstraction for CPU, memory, interrupts, I/O
- **Microkernel**: Core scheduling, IPC, minimal services
- **Executive**: Higher-level OS services and managers
- **User Mode**: Application environment and subsystems

</details>

## 🐛 Bug Tracking

We maintain transparent bug tracking to show project health:

- 📊 **Overall Progress**: 71% of issues resolved
- ✅ **Critical Issues**: 0 remaining
- 🔧 **Logic Errors**: 0 remaining
- 📝 **Full Issue Tracker**: [currentbugs.md](currentbugs.md)

> [!NOTE]
> Found a bug? Please [report it](https://github.com/chosentechies/GamerOS/issues) with detailed reproduction steps.

## 🤝 Contributing

We welcome contributions from developers of all skill levels!

### How to Contribute

1. **🍴 Fork the repository**
2. **🌿 Create your feature branch** (`git checkout -b feature/AmazingFeature`)
3. **💾 Commit your changes** (`git commit -m 'Add some AmazingFeature'`)
4. **📤 Push to the branch** (`git push origin feature/AmazingFeature`)
5. **🔀 Open a Pull Request**

### Contribution Ideas

- 🐛 **Bug Fixes**: Check [current bugs](currentbugs.md)
- ✨ **New Features**: Implement drivers, system calls, UI components
- 📖 **Documentation**: Improve guides and code comments
- 🧪 **Testing**: Add test cases and validation
- 🎨 **UI/UX**: Enhance the desktop environment

### Development Guidelines

- Follow existing code style and conventions
- Add comments for complex logic
- Update documentation for new features
- Ensure builds pass before submitting PRs
- Write clear commit messages

## 📚 Learning Resources

### Essential Reading

- 📖 [OSDev Wiki](https://wiki.osdev.org/) - Comprehensive OS development guide
- 📘 Intel 64 and IA-32 Architectures Software Developer Manuals
- 📕 "Operating System Concepts" by Silberschatz, Galvin, and Gagne
- 📗 "Computer Systems: A Programmer's Perspective" by Bryant and O'Hallaron

### Video Tutorials

- 🎥 [QEMU Complete Setup Guide](https://www.youtube.com/watch?v=HywXtRz0URE)
- 🎬 YouTube: Search "x86 OS development" for hands-on tutorials

### Community

- 💬 OSDev Forums - Active community of OS developers
- 🗨️ Reddit: r/osdev - Discussions and project showcases
- 💻 GitHub Topics: [#operating-system](https://github.com/topics/operating-system)

## 📄 License

This project is licensed under a custom license. See the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

**Created by** [Chosentechies](https://github.com/chosentechies)

**Inspired by:** Modern desktop operating systems and the pursuit of deep systems knowledge

**Special thanks to:**
- The OSDev community for invaluable resources
- Contributors to GCC, NASM, QEMU, and other open-source tools
- Everyone who has contributed code, ideas, and feedback

---

<div align="center">

**Made with ❤️ for learning, experimentation, and fun**

[⬆ Back to Top](#gameros)

</div>