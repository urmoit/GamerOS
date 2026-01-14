@echo off

REM Run GamerOS in QEMU (debug)

REM Check for QEMU
REM Check for QEMU in common locations
set QEMU_EXE=
if exist "C:\Program Files\qemu\qemu-system-x86_64.exe" (
    set QEMU_EXE="C:\Program Files\qemu\qemu-system-x86_64.exe"
) else if exist "C:\Program Files (x86)\qemu\qemu-system-x86_64.exe" (
    set QEMU_EXE="C:\Program Files (x86)\qemu\qemu-system-x86_64.exe"
) else if exist "C:\qemu\qemu-system-x86_64.exe" (
    set QEMU_EXE="C:\qemu\qemu-system-x86_64.exe"
) else (
    where qemu-system-x86_64 >nul 2>&1
    if %errorlevel% == 0 (
        set QEMU_EXE=qemu-system-x86_64
    ) else (
        echo ERROR: QEMU not found!
        echo Please install QEMU or update the path in this script.
        echo You can download QEMU from: https://www.qemu.org/download.html
        exit /b 1
    )
)

REM Check if kernel ISO exists
if not exist "dist\x86_64\kernel.iso" (
    echo ERROR: ISO file not found at dist\x86_64\kernel.iso
    echo Please build the kernel first with: build.bat
    exit /b 1
)

REM Run QEMU
%QEMU_EXE% -cdrom dist\x86_64\kernel.iso -monitor stdio