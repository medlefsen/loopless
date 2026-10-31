#!/usr/bin/env bash
# Builds the pieces of Syslinux that loopless links against.
# Run by CMake's Syslinux ExternalProject, from the Syslinux source tree.
set -euo pipefail

export LDFLAGS+=--no-dynamic-linker  # workaround for binutils 2.28 http://www.syslinux.org/wiki/index.php?title=Building
export EXTRA_CFLAGS=-fno-PIE         # to fix gpxe build

# Syslinux 6.04-pre2 predates GCC 10, which changed the default from -fcommon
# to -fno-common. Several of its headers rely on tentative definitions being
# merged (e.g. tcp_conn_ops in core/fs/pxe/pxe.h), so both the core and com32
# builds fail to link without this. mk/local.mk is included by every Syslinux
# sub-make, so setting it here applies everywhere.
cat > mk/local.mk <<'LOCALMK'
CC += -fcommon
LOCALMK

# loopless only needs two things out of this tree:
#
#   bios/libinstaller/{bootsect,ldlinux,ldlinuxc32}_bin.c
#   bios/mbr/gptmbr.bin
#
# so build `libinstaller` and `mbr` plus what they depend on (`core` and
# `com32`, with `codepage` and `lzo` beneath those) and nothing else. A full
# `make bios` additionally builds the DOS/Windows installers, memdisk, diag and
# the host utilities, none of which loopless uses and several of which no
# longer compile with a current toolchain -- the DOS installer fails to link
# against its freestanding libc, and win32/win64 want a mingw cross compiler.
make bios \
  BSUBDIRS="codepage com32 lzo core mbr libinstaller" \
  ISUBDIRS=""
