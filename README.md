
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
[![Version](https://img.shields.io/badge/version-1.0-blue.svg)](https://github.com/chosentechies/GamerOS)
[![Build](https://img.shields.io/badge/build-0.0.0.1200-orange.svg)](https://github.com/chosentechies/GamerOS)
[![License](https://img.shields.io/badge/license-Custom-red.svg)](https://github.com/chosentechies/GamerOS/blob/main/LICENSE)

> ⚠️ **WORK IN PROGRESS** ⚠️
>
> This operating system project is under heavy development. Features may change, break, or be incomplete at any time. The name "GamerOS" is a placeholder and will be changed upon release.
>
> **Current Build: 0.0.0.1200** | See [Changelog](changelog/Build0.0.0.1200.md) for details.

## About The Project

GamerOS is a 64-bit hobby operating system with a custom kernel, graphical desktop, and windowing system inspired by Windows 11. It runs on the x86_64 architecture and is written from scratch in C and x86-64 Assembly. The project aims to provide a modern desktop experience with direct hardware access, featuring a custom-built graphical user interface.

**Planned Features:**
*   Support for running Linux applications.
*   Compatibility layer for Windows applications.
*   Ability to run APK files (Android apps).

The latest ISO file can always be found in the `dist/x86_64` folder.

**Current Build:** 0.0.0.1200

## Features

*   **Custom 64-bit Kernel:** x86_64, ELF format, with no standard library dependencies.
*   **Graphical Desktop Environment:** Inspired by Windows 11, featuring:
    *   Taskbar, Start Menu, and System Tray
    *   Multiple movable windows (e.g., File Explorer, Settings)
    *   Desktop icons like "This PC" and "Recycle Bin"
*   **Graphics:** VGA (320x200x256) graphics mode 13h.
*   **Input:** Mouse cursor with simulated/keyboard movement and keyboard event handling.
*   **System:** Real-Time Clock (RTC) reading and display, and hardware interrupt controllers (PIC, IDT) setup.
*   **Build Environment:** Portable Makefile and Docker-based build environment for easy setup.
*   **Emulation:** QEMU virtualization for easy testing and development.

## Roadmap

*   [ ] Implement a proper memory manager.
*   [ ] Develop a file system.
*   [ ] Add more UI features and improve the desktop experience.
*   [ ] Networking stack.
*   [ ] Sound support.

See the [open issues](https://github.com/chosentechies/GamerOS/issues) for a full list of proposed features (and known issues).

## License

Copyright (c) 2025 Chosentechies. All rights reserved.

This project is licensed under a custom license. See `LICENSE` for more information.

## Credits

*   **Author:** Chosentechies
*   **Inspired by:** Modern OS UIs
