#!/usr/bin/env bash
# Configures and builds loopless into the given build directory.
# Expects the build dependencies from install-build-deps.sh to be present.
set -euo pipefail

build_dir="${1:?usage: do-build.sh BUILD_DIR}"

cd "$(dirname "$0")"
src_dir="$PWD"

mkdir -p "$build_dir"
cd "$build_dir"

cmake -D CMAKE_CMD=cmake -D CMAKE_BUILD_TYPE=Release "$src_dir"
make VERBOSE=1
