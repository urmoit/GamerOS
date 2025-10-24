FROM ubuntu:20.04

# Set timezone to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Tallinn

# Install only essential packages quickly
RUN apt-get update && apt-get install -y \
    build-essential \
    nasm \
    xorriso \
    grub-pc-bin \
    qemu-system-x86 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install cross-compiler from pre-built binaries
RUN curl -L https://musl.cc/x86_64-linux-musl-cross.tgz | tar -xzC /usr/local && \
    ln -s /usr/local/x86_64-linux-musl-cross/bin/* /usr/local/bin/

WORKDIR /root/env
