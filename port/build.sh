#!/usr/bin/env bash
# Build the ROM and the PC port.
# Run from anywhere in an MSYS2 UCRT64 shell (or via build.bat):
#   port/build.sh            build the game (no interpreter) in port/build-game
#   port/build.sh run        build the game and start it
#   port/build.sh dev        build the development build (interpreter, tests,
#                            recording) in port/build
#   port/build.sh dev run    build that and start it
# Set RGBDS to the RGBDS folder if it isn't C:\rgbds (only used the first
# time, when a build is configured).
set -euo pipefail

cd "$(dirname "$0")/.."

dir=port/build-game
options=(-DWL3_INTERPRETER=OFF)
if [ "${1:-}" = "dev" ]; then
	dir=port/build
	options=()
	shift
fi

if [ ! -f "$dir/CMakeCache.txt" ]; then
	echo "== Configuring $dir"
	cmake -B "$dir" -S port -G Ninja -DRGBDS="${RGBDS:-/c/rgbds/}" "${options[@]}"
fi

echo "== Building $dir"
cmake --build "$dir"

echo "== Done"
if [ "${1:-}" = "run" ]; then
	"./$dir/wl3.exe"
fi
