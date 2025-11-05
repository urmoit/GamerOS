FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    nasm \
    xorriso \
    grub-pc-bin \
    grub-common \
    binutils \
    qemu-system-x86 \
    && rm -rf /var/lib/apt/lists/*

# Install x86_64-elf cross-compiler
RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    make \
    binutils \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    texinfo \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root

# Build cross-compiler (simplified - using system gcc with target flags)
# For a full bootstrap, you'd build binutils and gcc from source
# This approach uses system gcc with appropriate flags

CMD ["/bin/bash"]

