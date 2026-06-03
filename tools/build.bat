@echo off

@echo off

REM Build GamerOS natively on Windows
make build-x86_64
if %errorlevel% neq 0 (
    echo Build FAILED with error code %errorlevel%
    pause
    exit /b %errorlevel%
)
echo.
echo Build complete!
pause