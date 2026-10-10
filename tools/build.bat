@echo off

REM Build GamerOS natively on Windows.
REM Run make from the tools directory: the Makefile uses ../src style paths.
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
echo Build complete!
echo ISO: dist\x86_64\kernel.iso
pause