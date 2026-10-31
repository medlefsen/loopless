#!/usr/bin/env bash
# Smoke test: exercises the full bootable-image path against a scratch image.
set -euo pipefail

cd "$(dirname "$0")"

LOOPLESS="${LOOPLESS:-build_linux/loopless}"

rm -f disk.img
$LOOPLESS disk.img create 1000000000
$LOOPLESS disk.img create-gpt
$LOOPLESS disk.img create-part
$LOOPLESS disk.img part-info
$LOOPLESS disk.img:1 mkfs
echo 'SAY Hello World!' | $LOOPLESS disk.img:1 write /syslinux.cfg 600
$LOOPLESS disk.img:1 syslinux
$LOOPLESS disk.img:1 set-bootable true
$LOOPLESS disk.img install-mbr

# Verify the file we wrote is actually readable back out of the image.
$LOOPLESS disk.img:1 ls
written="$($LOOPLESS disk.img:1 read /syslinux.cfg)"
if [ "$written" != 'SAY Hello World!' ]; then
  echo >&2 "error: read back '$written', expected 'SAY Hello World!'"
  exit 1
fi

rm -f disk.img
echo "OK"
