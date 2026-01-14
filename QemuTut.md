# QEMU Tutorial for GamerOS

This guide provides comprehensive instructions for using QEMU with GamerOS, including installation, configuration, and troubleshooting.

## Table of Contents

- [Installation](#installation)
- [Basic Usage](#basic-usage)
- [Advanced Configuration](#advanced-configuration)
- [Debugging](#debugging)
- [Troubleshooting](#troubleshooting)
- [Video Tutorial](#video-tutorial)

## Installation

### Windows Installation

1. **Download QEMU**: Get the latest version from [QEMU Official Website](https://www.qemu.org/download/#windows)
2. **Install**: Run the installer and ensure QEMU is added to your PATH
3. **Verify**: Open Command Prompt and run:
   ```cmd
   qemu-system-x86_64 --version
   ```

### Linux Installation

```bash
# Ubuntu/Debian
sudo apt-get install qemu-system-x86

# Fedora
sudo dnf install qemu-system-x86

# Arch Linux
sudo pacman -S qemu
```

### macOS Installation

```bash
brew install qemu
```

## Basic Usage

### Running GamerOS

```cmd
run-qemu.bat
```

This script automatically:
- Locates QEMU installation
- Checks for the ISO file
- Configures QEMU with optimal settings for GamerOS
- Disables SMM (System Management Mode) for compatibility
- Enables serial output for debugging

### Manual QEMU Command

```bash
qemu-system-x86_64 \
  -cdrom dist/x86_64/kernel.iso \
  -no-reboot -no-shutdown \
  -machine pc,accel=tcg,smm=off \
  -cpu qemu64 \
  -serial stdio \
  -D qemu-debug.log
```

## Advanced Configuration

### QEMU Options Explained

- `-cdrom`: Specifies the bootable ISO image
- `-no-reboot -no-shutdown`: Prevents automatic reboot/shutdown on errors
- `-machine pc,accel=tcg,smm=off`: Uses TCG accelerator and disables SMM
- `-cpu qemu64`: Specifies 64-bit CPU emulation
- `-serial stdio`: Redirects serial output to console
- `-D qemu-debug.log`: Writes debug logs to file

### Memory Configuration

```bash
qemu-system-x86_64 -m 2048 ...  # 2GB RAM
```

### Display Configuration

```bash
qemu-system-x86_64 -vga std ...  # Standard VGA
qemu-system-x86_64 -vga vmware ...  # VMware SVGA
```

## Debugging

### Debug Mode

Use the debug script:

```cmd
debug.bat
```

This provides a QEMU monitor console for advanced debugging.

### Common Debug Commands

- `info registers`: Show CPU register state
- `xp /10i $pc`: Show next 10 instructions
- `cont`: Continue execution
- `quit`: Exit QEMU

### Serial Debugging

GamerOS outputs debug information to the serial port, which is redirected to the console with `-serial stdio`.

## Troubleshooting

### Common Issues

1. **QEMU Not Found**: Ensure QEMU is installed and in your PATH
2. **ISO Not Found**: Build the project first with `build.bat`
3. **Graphics Issues**: Try different VGA modes or update QEMU
4. **Performance Issues**: Use `-machine pc,accel=tcg` for better performance

### Error Messages

- **"No rule to make target 'build-iso'"**: Use `build.bat` instead of `build-iso.bat`
- **"QEMU not found"**: Install QEMU or update the script with the correct path
- **Graphics display issues**: Try `-vga std` or update your QEMU version

## Video Tutorial

For a visual guide, watch this comprehensive QEMU setup tutorial:

[QEMU Tutorial Video](https://www.youtube.com/watch?v=HywXtRz0URE)

## Additional Resources

- [QEMU Official Documentation](https://www.qemu.org/docs/master/)
- [QEMU Command Line Options](https://www.qemu.org/docs/master/system/invocation.html)
- [OSDev Wiki QEMU Guide](https://wiki.osdev.org/QEMU)