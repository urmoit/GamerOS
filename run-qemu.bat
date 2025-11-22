@echo off

REM Run GamerOS in QEMU

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
        echo ========================================
        echo ERROR: QEMU not found!
        echo ========================================
        echo.
        echo Please install QEMU for Windows:
        echo.
        echo 1. Download from: https://www.qemu.org/download/#windows
        echo 2. Or use Chocolatey: choco install qemu
        echo 3. Or use Winget: winget install qemu.qemu
        echo.
        echo After installation, add QEMU to your PATH or update this script
        echo with the correct path to qemu-system-x86_64.exe
        echo.
        exit /b 1
    )
)

REM Check if kernel ISO exists
if not exist "dist\x86_64\kernel.iso" (
    echo ERROR: ISO file not found at dist\x86_64\kernel.iso
    echo Please build the kernel first with: build.bat
    exit /b 1
)

REM Run QEMU with debugging options to prevent immediate exit and disable SMM
REM Serial output goes to console, default VGA display
echo Starting QEMU with ISO: dist\x86_64\kernel.iso
%QEMU_EXE% -cdrom dist\x86_64\kernel.iso -no-reboot -no-shutdown -d int -machine pc,accel=tcg,smm=off -cpu qemu64 -serial stdio
