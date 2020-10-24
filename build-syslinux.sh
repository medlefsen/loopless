#!/usr/bin/env bash
export LDFLAGS+=--no-dynamic-linker  # workaround for binutils 2.28 http://www.syslinux.org/wiki/index.php?title=Building
export EXTRA_CFLAGS=-fno-PIE   # to fix gpxe build
make bios
