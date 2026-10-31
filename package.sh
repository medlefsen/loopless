#!/usr/bin/env bash
# Packages a built loopless binary into a release tarball under dist/.
#
#   ./build.sh && ./package.sh
#
# Produces dist/loopless-<version>-x86_64-linux.tar.gz and a SHA256SUMS file.
set -euo pipefail

cd "$(dirname "$0")"

BUILD_DIR="${BUILD_DIR:-build_linux}"
binary="$BUILD_DIR/loopless"

if [ ! -x "$binary" ]; then
  echo >&2 "error: $binary not found or not executable; run ./build.sh first"
  exit 1
fi

version="$(< VERSION)"
target="x86_64-linux"
name="loopless-${version}-${target}"

rm -rf "dist/$name" "dist/$name.tar.gz"
mkdir -p "dist/$name"

install -m 0755 "$binary" "dist/$name/loopless"
install -m 0644 README.md COPYING "dist/$name/"

# Strip the shipped copy; the build tree keeps its symbols for debugging.
if command -v strip >/dev/null 2>&1; then
  strip "dist/$name/loopless"
else
  echo >&2 "warning: strip not found, shipping an unstripped binary"
fi

tar -czf "dist/$name.tar.gz" -C dist "$name"
rm -rf "dist/$name"

# Also ship the bare binary -- sysforge vendors it directly.
tar -xzOf "dist/$name.tar.gz" "$name/loopless" > "dist/loopless-${version}-${target}"
chmod 0755 "dist/loopless-${version}-${target}"

( cd dist && sha256sum "$name.tar.gz" "loopless-${version}-${target}" > SHA256SUMS )

echo "packaged:"
ls -la dist/
