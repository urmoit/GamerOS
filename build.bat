@echo off

REM Build GamerOS

REM Build the Docker image
docker build -t gameros-buildenv buildenv

REM Run the build
docker run --rm -v "%cd%:/root/env" gameros-buildenv bash -c "cd /root/env && make build-x86_64"