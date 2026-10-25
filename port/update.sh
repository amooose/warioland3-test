#!/usr/bin/env bash
# Pull the latest changes and build the PC port.
# Run from anywhere in an MSYS2 UCRT64 shell (or via update.bat):
#   port/update.sh          pull and build
#   port/update.sh run      pull, build and start the game
# Set RGBDS to the RGBDS folder if it isn't C:\rgbds (only used the first
# time, when the build is configured).
set -euo pipefail

cd "$(dirname "$0")/.."

echo "== Pulling latest changes"
git pull --ff-only

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
