@echo off

REM Build GamerOS

REM Build the Docker image
docker build -t gameros-buildenv ../config/buildenv

REM Run the build from tools directory where Makefile is located
docker run --rm -v "%cd%:/root/env" gameros-buildenv bash -c "cd /root/env/tools && make build-x86_64"

echo.
echo Build complete!
pause