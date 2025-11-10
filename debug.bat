@echo off

REM Run GamerOS in QEMU (debug)

REM Check for QEMU
if not exist "C:\Program Files\qemu\qemu-system-x86_64.exe" (
    echo ERROR: QEMU not found at C:\Program Files\qemu\qemu-system-x86_64.exe
    echo Please install QEMU or update the path in this script.
    echo You can download QEMU from: https://www.qemu.org/download.html
    exit /b 1
)

REM Check if kernel ISO exists
if not exist "dist\x86_64\kernel.iso" (
    echo ERROR: ISO file not found at dist\x86_64\kernel.iso
    echo Please build the kernel first with: build.bat
    exit /b 1
)

REM Run QEMU
"C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom dist\x86_64\kernel.iso -monitor stdio