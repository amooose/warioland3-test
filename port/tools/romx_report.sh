#!/bin/sh
# What still reads ROM data through the bank mapped: each recording's trace
# check with --romx-report, summed by label. Prints "count distinct label
# kind reader", most read first (kind: code, a label with a hook, whose reads are
# its own data or jump tables; data, anything else; reader: the routine
# running at one of its reads, by the pc native code last set). Modernisation's
# worklist for ROM data as views and then no bank switching.
#   tools/romx_report.sh [OUT]   (run from port/ after the quick tier, which
#                                 writes build/traces/)
cd "$(dirname "$0")/.." || exit 1
WL3=${WL3:-./build/wl3}
OUT=${1:-build/romx_report.txt}
tmp=build/romx_report.d
rm -rf "$tmp" && mkdir -p "$tmp"
for i in tests/inputs/*.input; do
	name=$(basename "$i" .input)
	[ -f "build/traces/$name.trace" ] || { echo "no trace for $name (run the quick tier)" >&2; continue; }
	"$WL3" --headless --input "$i" --trace-check "build/traces/$name.trace" \
		--romx-report "$tmp/$name.txt" > "$tmp/$name.log" 2>&1 ||
		echo "$name: the trace check failed (counts still summed)" >&2
done
cat "$tmp"/*.txt 2>/dev/null | awk '!/^#/ && NF == 5 { c[$3] += $1; if ($2 > d[$3]) d[$3] = $2; k[$3] = $4; r[$3] = $5 }
	END { for (l in c) print c[l], d[l], l, k[l], r[l] }' | sort -k1,1nr > "$OUT"
echo "$(wc -l < "$OUT") labels read through the bank mapped;" \
	"$(awk '$4 == "data"' "$OUT" | wc -l) data, $(awk '$4 == "code"' "$OUT" | wc -l) code"
