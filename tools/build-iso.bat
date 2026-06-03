@echo off
echo Building GamerOS ISO...
echo.
REM Build the OS image natively on Windows
make build-x86_64
if %errorlevel% neq 0 (
    echo.
    echo Build FAILED with error code %errorlevel%
    pause
    exit /b %errorlevel%
)
echo.
echo ========================================
echo Build complete!
echo ISO created at: dist\x86_64\kernel.iso
echo ========================================
pause
