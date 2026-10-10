@echo off

REM Run GamerOS in QEMU with the monitor attached.

REM Check for QEMU in common locations.
set QEMU_EXE=
if exist "C:\Program Files\qemu\qemu-system-x86_64.exe" (
    set QEMU_EXE="C:\Program Files\qemu\qemu-system-x86_64.exe"
) else if exist "C:\Program Files (x86)\qemu\qemu-system-x86_64.exe" (
    set QEMU_EXE="C:\Program Files (x86)\qemu\qemu-system-x86_64.exe"
) else if exist "C:\qemu\qemu-system-x86_64.exe" (
    set QEMU_EXE="C:\qemu\qemu-system-x86_64.exe"
) else (
    REM Use `if not errorlevel 1` rather than `if %errorlevel% == 0`:
    REM inside a parenthesised block %errorlevel% is expanded when the block
    REM is parsed, so it reported the value from before `where` ran.
    where qemu-system-x86_64 >nul 2>&1
    if not errorlevel 1 (
        set QEMU_EXE=qemu-system-x86_64
    ) else (
        echo ERROR: QEMU not found!
        echo Please install QEMU or update the path in this script.
        echo You can download QEMU from: https://www.qemu.org/download.html
        exit /b 1
    )
)

REM Resolve paths relative to the repo root, not the current directory.
set ROOT=%~dp0..
set ISO_PATH=%ROOT%\dist\x86_64\kernel.iso

if not exist "%ISO_PATH%" (
    echo ERROR: ISO file not found at %ISO_PATH%
    echo Please build the kernel first with: build.bat
    exit /b 1
)

REM Run QEMU
%QEMU_EXE% -cdrom "%ISO_PATH%" -monitor stdio