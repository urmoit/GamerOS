@echo off
echo Building GamerOS ISO...
echo.
docker run --rm -v "%cd%:/root/env" gameros-buildenv bash -c "cd /root/env && make build-x86_64"
if %errorlevel% neq 0 (
    echo.
    echo Build FAILED with error code %errorlevel%
    pause
    exit /b %errorlevel%
)
echo.
echo ========================================
echo Build!
echo ISO created at: dist\x86_64\kernel.iso
echo ========================================
pause
