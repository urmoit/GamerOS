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
    REM `if not errorlevel 1` instead of `if %errorlevel% == 0`: inside a
    REM parenthesised block %errorlevel% is expanded at parse time and would
    REM report the value from before `where` ran.
    where qemu-system-x86_64 >nul 2>&1
    if not errorlevel 1 (
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

REM Resolve paths relative to the repo root so the script works from anywhere.
set ROOT=%~dp0..
set ISO_PATH=%ROOT%\dist\x86_64\kernel.iso

if not exist "%ISO_PATH%" (
    echo ERROR: ISO file not found at %ISO_PATH%
    echo Please build the kernel first with: build.bat
    exit /b 1
)

REM Run QEMU with debugging options to prevent immediate exit and disable SMM
REM Serial output goes to console, default VGA display
REM Create/overwrite log file (deletes old one if exists)
set LOG_PATH=%ROOT%\qemu.log
set DEBUG_LOG_PATH=%ROOT%\qemu-debug.log

REM The USB HID stack drives UHCI controllers, and it reaches devices attached
REM directly to a root hub port. QEMU's xHCI is a different interface and is not
REM supported, so attach each USB device to its own UHCI controller's first
REM port. Putting two devices on one controller makes QEMU chain the second one
REM behind an auto-created USB hub, which the driver cannot reach.
set USB_OPTS=-usb -device piix3-usb-uhci,id=u1 -device piix3-usb-uhci,id=u2 -device usb-kbd,bus=u1.0 -device usb-mouse,bus=u2.0

echo Starting QEMU with ISO: %ISO_PATH%
if exist "%LOG_PATH%" (
    echo Overwriting existing qemu.log
    del "%LOG_PATH%" >nul 2>&1
)
echo Creating new qemu.log file...
echo QEMU Boot Log - %date% %time% > "%LOG_PATH%"
echo ======================================== >> "%LOG_PATH%"
echo Starting QEMU with ISO: %ISO_PATH% >> "%LOG_PATH%"
echo Command: %QEMU_EXE% -cdrom "%ISO_PATH%" -no-reboot -no-shutdown -machine pc,accel=tcg,smm=off -cpu max %USB_OPTS% -serial stdio -d cpu_reset,int,guest_errors -D "%DEBUG_LOG_PATH%" >> "%LOG_PATH%"
echo. >> "%LOG_PATH%"

REM Run QEMU and capture output. QEMU internal debug log goes to qemu-debug.log, main output goes to qemu.log
%QEMU_EXE% -cdrom "%ISO_PATH%" -no-reboot -no-shutdown -machine pc,accel=tcg,smm=off -cpu max %USB_OPTS% -serial stdio -d cpu_reset,int,guest_errors -D "%DEBUG_LOG_PATH%"