#!/usr/bin/env bash
# Build the ROM and the PC port.
# Run from anywhere in an MSYS2 UCRT64 shell (or via build.bat):
#   port/build.sh          build
#   port/build.sh run      build and start the game
# Set RGBDS to the RGBDS folder if it isn't C:\rgbds (only used the first
# time, when the build is configured).
set -euo pipefail

cd "$(dirname "$0")/.."

if [ ! -f port/build/CMakeCache.txt ]; then
	echo "== Configuring the build"
	cmake -B port/build -S port -G Ninja -DRGBDS="${RGBDS:-/c/rgbds/}"
fi

echo "== Building"
cmake --build port/build

echo "== Done"
if [ "${1:-}" = "run" ]; then
	./port/build/wl3.exe
fi
