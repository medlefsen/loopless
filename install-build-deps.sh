#!/usr/bin/env bash
# Installs the Debian packages needed to build loopless from source.
# Used by both build.sh (local container build) and the GitHub Actions
# workflows, so the dependency list has a single definition.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update

# gcc-multilib is required because syslinux's `make bios` builds the 32-bit
# com32 modules, and python-is-python3 because syslinux's cmenu Makefile
# invokes `python` rather than `python3`. xxd lives in vim-common on bullseye
# and in its own package from bookworm on, so try both.
apt-get install -y --no-install-recommends \
  autoconf \
  automake \
  bzip2 \
  ca-certificates \
  cmake \
  g++ \
  gcc \
  gcc-multilib \
  git \
  libtool \
  make \
  nasm \
  perl \
  pkg-config \
  python3 \
  python-is-python3 \
  uuid-dev \
  zlib1g-dev

apt-get install -y --no-install-recommends xxd \
  || apt-get install -y --no-install-recommends vim-common

apt-get clean
rm -rf /var/lib/apt/lists/*
