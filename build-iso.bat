@echo off
echo Building GamerOS ISO...
docker run --rm -v "%cd%:/root/env" gameros-buildenv /bin/bash -c "cd /root/env && make build-iso ARCH=x86_64"
echo Build complete!