@echo off

REM Run GamerOS in QEMU (debug)

REM Check for QEMU
where qemu-system-x86_64 >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: QEMU not found in PATH.
    echo Please install QEMU and add it to your PATH.
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
qemu-system-x86_64 -cdrom dist\x86_64\kernel.iso -monitor stdio