@echo off
echo Building GamerOS ISO...
echo.

REM Run make from the tools directory so ../src style paths resolve.
pushd "%~dp0"
make build-x86_64
set BUILD_EXIT=%errorlevel%
popd

if %BUILD_EXIT% neq 0 (
    echo.
    echo Build FAILED with error code %BUILD_EXIT%
    pause
    exit /b %BUILD_EXIT%
)

echo.
echo ========================================
echo Build complete!
echo ISO created at: dist\x86_64\kernel.iso
echo ========================================
pause