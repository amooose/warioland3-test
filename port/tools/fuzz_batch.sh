#!/bin/sh
# Fuzz routines against every test input and print one line per
# (routine, input) plus the first lines of any failure.
#   tools/fuzz_batch.sh RoutineA RoutineB ...    (run from port/)
# Each input is replayed once for all the routines (the replay takes far
# longer than fuzzing one routine), JOBS inputs at a time (default 4).
# Set WL3 to point at another build.
#
# A line reads "<routine> <input>: PASS|FAIL (passed/failed/inconclusive,
# asm cover)". NOT FOUND means no hook has that name (check for typos and
# trailing \r). A PASS with 0 passed checks verified nothing.
cd "$(dirname "$0")/.." || exit 1
WL3=${WL3:-./build/wl3}
JOBS=${JOBS:-4}
[ $# -gt 0 ] || { echo "usage: $0 Routine..." >&2; exit 1; }
list=$(printf '%s\n' "$@" | tr -d '\r' | paste -sd, -)
for i in tests/inputs/*.input; do
	echo "$i"
done | xargs -P "$JOBS" -L1 sh -c '
	list=$0 input=$1
	name=$(basename "$input" .input)
	out=$('"$WL3"' --fuzz --input "$input" --fuzz-routine "$list" 2>&1)
	echo "$list" | tr , "\n" | while read -r r; do
		# routine seeds samples passed failed inconclusive cover
		row=$(echo "$out" | awk -v r="$r" "\$1 == r && NF >= 7 { print; exit }")
		if [ -z "$row" ]; then
			echo "$r $name: NOT FOUND"
			continue
		fi
		set -- $row
		verdict=PASS
		[ "$5" != 0 ] && verdict=FAIL
		echo "$r $name: $verdict ($4/$5/$6, cover $7)"
	done
	echo "$out" | grep -A6 "^FAIL" | head -16
	echo "$out" | tail -1 | grep -q "fuzz: \(PASS\|FAIL\)" || echo "$name: no fuzz result: $(echo "$out" | tail -1)"
' "$list"
