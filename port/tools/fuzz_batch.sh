#!/bin/sh
# Fuzz routines against every test input, 4 at a time, and print one line
# per (routine, input) plus the first lines of any failure.
#   tools/fuzz_batch.sh RoutineA RoutineB ...    (run from port/)
# Set JOBS to change the parallelism, WL3 to point at another build.
cd "$(dirname "$0")/.." || exit 1
WL3=${WL3:-./build/wl3}
JOBS=${JOBS:-4}
for r in "$@"; do
	for i in tests/inputs/*.input; do
		echo "$r $i"
	done
done | xargs -P "$JOBS" -L1 sh -c '
	out=$('"$WL3"' --fuzz --input "$1" --fuzz-routine "$0" 2>&1)
	echo "$0 $(basename "$1" .input): $(echo "$out" | tail -1)"
	echo "$out" | grep -A6 "^FAIL" | head -8
'
