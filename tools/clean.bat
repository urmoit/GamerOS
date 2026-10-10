@echo off

REM Clean GamerOS build artifacts.
REM Run make from the tools directory so ../src style paths resolve.
pushd "%~dp0"
make clean
set CLEAN_EXIT=%errorlevel%
popd

if %CLEAN_EXIT% neq 0 (
    echo Clean FAILED with error code %CLEAN_EXIT%
    exit /b %CLEAN_EXIT%
)
exit /b 0