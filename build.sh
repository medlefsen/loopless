#!/usr/bin/env bash
# Builds the linux x86_64 loopless binary inside a pinned Debian container so
# release builds are reproducible and have a predictable glibc floor.
#
#   ./build.sh            # build into build_linux/
#   BUILD_DIR=out ./build.sh
#
# Set CONTAINER_ENGINE=docker to use docker instead of podman.
set -euo pipefail

cd "$(dirname "$0")"

# Pinned so release builds are reproducible. Debian 12 ships glibc 2.36, which
# is the floor for the resulting binary -- it runs on that glibc or newer.
BUILD_IMAGE="${BUILD_IMAGE:-docker.io/library/debian:bookworm}"
BUILD_DIR="${BUILD_DIR:-build_linux}"

if [ -n "${CONTAINER_ENGINE:-}" ]; then
  engine="$CONTAINER_ENGINE"
elif command -v podman >/dev/null 2>&1; then
  engine=podman
elif command -v docker >/dev/null 2>&1; then
  engine=docker
else
  echo >&2 "error: neither podman nor docker found"
  exit 1
fi

# -t only when attached to a terminal, so this works unattended in CI.
tty_flags=()
if [ -t 0 ]; then
  tty_flags=(-t)
fi

mount_opts=""
if [ "$engine" = podman ]; then
  mount_opts=":Z"
fi

exec "$engine" run --rm "${tty_flags[@]}" \
  -v "$PWD:/workdir$mount_opts" \
  -w /workdir \
  "$BUILD_IMAGE" \
  bash -c "./install-build-deps.sh && ./do-build.sh '$BUILD_DIR'"
