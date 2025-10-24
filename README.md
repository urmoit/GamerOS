> ⚠️ **WORK IN PROGRESS** ⚠️
>
> **This operating system project is under heavy development. Features may change, break, or be incomplete at any time.**
>
> _Note: The name "GamerOS" is a placeholder and will be changed once the OS is ready for release._
>
> **Planned: This OS aims to support running Linux applications, Windows applications, and APK files (Android apps) in the future.**
>
> The latest ISO file can always be found in the [dist/x86_64](dist/x86_64) folder.

# GamerOS: A Simple x86_64 Operating System with Windows 11-Like Desktop

GamerOS is a 64-bit hobby operating system with a custom kernel, graphical desktop, and windowing system inspired by Windows 11. It runs on x86_64, supports basic keyboard and mouse (emulated/simulated), and demonstrates UI elements such as windows, taskbar, start menu, and more—all written from scratch in C and x86-64 assembly with direct hardware access using VGA graphics.

## Features
- Custom 64-bit kernel (x86_64, ELF format, no standard library)
- Graphical desktop environment, inspired by Windows 11
  - Taskbar, Start menu, system tray
  - Multiple movable windows (File Explorer, Settings, etc.)
  - Simple desktop icons ("This PC", "Recycle Bin", etc.)
- VGA (320x200x256) graphics mode 13h
- Mouse cursor with simulated/keyboard move
- Keyboard event handling and interrupt setup
- Real-Time Clock reading and display
- Hardware interrupt controllers (PIC, IDT) setup
- Portable Makefile and Docker-based build environment
- QEMU virtualization for easy emulation

## Credits
- Author: Chosentechies
- Inspired by modern OS UIs

## License
Copyright (c) 2025 Chosentechies. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to view, use, and execute the Software for personal, non-commercial purposes only, subject to the following conditions:

1. You may NOT modify, adapt, merge, publish, distribute, sublicense, or create derivative works of the Software, in whole or in part, without the express, prior, written permission of Chosentechies.
2. You may NOT use the Software, in whole or in part, for any commercial purpose without explicit permission from Chosentechies.
3. Any requests for permissions regarding modification, redistribution, or commercial use must be directed to Chosentechies. Approval or denial is at the sole discretion of Chosentechies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

All rights not expressly granted herein are reserved by Chosentechies. For permissions and inquiries, contact: Chosentechies

