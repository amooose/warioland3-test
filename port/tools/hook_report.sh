#!/bin/sh
# Which hooks the game still reaches by address, over every recording: each
# input's trace check (the ported routines, at the asm's timing) with
# --hook-report, summed. Prints "hook calls by_call by_jump by_irq", most
# called first. Stage 5's to-do list: a routine nothing reaches through its
# hook (only the fuzzer and difftests do) needs none in the game build.
#   tools/hook_report.sh [OUT]   (run from port/ after the quick tier, which
#                                 writes build/traces/)
cd "$(dirname "$0")/.." || exit 1
WL3=${WL3:-./build/wl3}
OUT=${1:-build/hook_report.txt}
tmp=build/hook_report.d
rm -rf "$tmp" && mkdir -p "$tmp"
for i in tests/inputs/*.input; do
	name=$(basename "$i" .input)
	[ -f "build/traces/$name.trace" ] || { echo "no trace for $name (run the quick tier)" >&2; continue; }
	"$WL3" --headless --input "$i" --trace-check "build/traces/$name.trace" \
		--hook-report "$tmp/$name.txt" > "$tmp/$name.log" 2>&1 ||
		echo "$name: the trace check failed (counts still summed)" >&2
done
cat "$tmp"/*.txt 2>/dev/null | awk '!/^#/ && NF == 5 { c[$1] += $2; k[$1] += $3; j[$1] += $4; q[$1] += $5 }
	END { for (h in c) print h, c[h], k[h], j[h], q[h] }' | sort -k2,2nr -k1,1 > "$OUT.tmp"
{ echo "# hook calls by_call by_jump by_irq"; cat "$OUT.tmp"; } > "$OUT" && rm -f "$OUT.tmp"
echo "$(($(wc -l < "$OUT") - 1)) hooks ran; by kind (hooks with any):"
awk '!/^#/ { if ($3) a++; if ($4) b++; if ($5) c++ } END { print "  native call by address: " a "\n  jump/ret/fallthrough: " b "\n  interrupt: " c }' "$OUT"
