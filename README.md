
<div align="center">
  <h1 align="center">GamerOS</h1>
  <p align="center">
    A simple x86_64 hobby operating system with a Windows 11-like desktop environment, built from scratch.
    <br />
    <a href="https://github.com/chosentechies/GamerOS/issues">Report Bug</a>
    ·
    <a href="https://github.com/chosentechies/GamerOS/issues">Request Feature</a>
  </p>
</div>

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/chosentechies/GamerOS)
[![Version](https://img.shields.io/badge/version-1.1-blue.svg)](https://github.com/chosentechies/GamerOS)
[![Build](https://img.shields.io/badge/build-0.0.0.1420-orange.svg)](https://github.com/chosentechies/GamerOS)
[![License](https://img.shields.io/badge/license-Custom-red.svg)](https://github.com/chosentechies/GamerOS/blob/main/LICENSE)

> ⚠️ **WORK IN PROGRESS** ⚠️
>
> This operating system project is under heavy development. Features may change, break, or be incomplete at any time. The name "GamerOS" is a placeholder and will be changed upon release.
>
> **Current Build: 0.0.0.1420** | See [Changelog](changelog/Build0.0.0.1420.md) for details.

## 🚀 Getting Started

### Prerequisites

Before you begin, ensure you have the following installed:

- **[Docker](https://www.docker.com/get-started)** - For cross-compilation environment
- **[QEMU](https://www.qemu.org/download/)** - For running and testing the OS

### 📚 Learning Resources

- **QEMU Tutorial**: [Complete QEMU Setup Guide](https://www.youtube.com/watch?v=HywXtRz0URE) - Learn how to install and use QEMU for OS development
- **OS Development**: Check out [OSDev Wiki](https://wiki.osdev.org/) for comprehensive documentation

### 🛠️ Quick Start

1. **Clone the repository**
   ```bash
   git clone https://github.com/chosentechies/GamerOS.git
   cd GamerOS
   ```

2. **Build the OS**
   ```batch
   build.bat
   ```

3. **Run in QEMU**
   ```batch
   run-qemu.bat
   ```

4. **Debug mode** (optional)
   ```batch
   debug.bat
   ```

5. **Clean build artifacts** (when needed)
   ```batch
   clean.bat
   ```

## 📖 About The Project

**GamerOS** is a 64-bit hobby operating system built entirely from scratch, featuring a Windows 11-inspired desktop environment. Written in C and x86-64 Assembly, it provides direct hardware access and a modern graphical user interface.

### 🎯 Key Features

- **Custom 64-bit Kernel** - No standard library dependencies
- **Graphical Desktop** - Windows 11-style UI with taskbar, start menu, and windows
- **Multiple Resolutions** - Support for 320x200 to 1024x768 with 32-bit color depth
- **Hardware Integration** - Keyboard, mouse, RTC, and interrupt handling
- **Docker Build System** - Cross-platform compilation environment


### 📁 Project Structure

```
GamerOS/
├── src/                    # Source code
│   ├── intf/              # Interfaces and headers
│   └── impl/              # Implementations
├── build/                 # Build artifacts
├── dist/                  # Distribution files
├── targets/               # Build targets
└── changelog/             # Version history
```

**Latest Release**: Build 0.0.0.1420 | [📋 Changelog](changelog/Build0.0.0.1420.md)

## ✨ Current Features

### 🖥️ Desktop Environment
- **Windows 11-inspired UI** with taskbar and start menu
- **Movable windows** with basic window management
- **Desktop icons** and system tray
- **Clock display** with real-time updates

### 🎨 Graphics System
- **Multiple video modes**: VGA and VESA support
- **Resolution range**: 320x200 to 1024x768
- **Color depth**: Up to 32-bit RGBA
- **Drawing primitives**: Lines, circles, rectangles, triangles
- **Software rendering** with double buffering

### ⌨️ Input & Hardware
- **PS/2 keyboard** with scancode handling
- **PS/2 mouse** with cursor movement
- **Real-Time Clock** (RTC) integration
- **Interrupt handling** (PIC/IDT setup)

### 🏗️ Development
- **Docker build environment** for cross-compilation
- **Makefile-based build system**
- **QEMU integration** for testing and debugging
- **Modular architecture** with clean separation


## License

Copyright (c) 2025 Chosentechies. All rights reserved.

This project is licensed under a custom license. See `LICENSE` for more information.

## Credits

*   **Author:** Chosentechies
*   **Inspired by:** Modern OS UIs
