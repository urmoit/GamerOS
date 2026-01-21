@echo off
echo Building GamerOS ISO...
docker run --rm -v "%cd%:/root/env" gameros-buildenv bash -c "cd /root/env && make build-x86_64"
echo.
echo Build complete!
pause