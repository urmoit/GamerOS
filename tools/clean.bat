@echo off

REM Clean GamerOS build artifacts

REM Run the clean command
docker run --rm -v "%cd%:/root/env" gameros-buildenv bash -c "cd /root/env && make clean"