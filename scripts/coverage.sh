#!/usr/bin/env bash
# Measures how much of elpris_led/elpris_logic.h the unit tests run, and fails below the limits.
# Usage: scripts/coverage.sh   (or: make coverage)
# Needs only g++ and gcov, which come with GCC. The limits are explained in AGENTS.md.
set -euo pipefail

MIN_LINES=95      # percent of lines run at least once
MIN_BRANCHES=90   # percent of branches taken at least once

cd "$(dirname "$0")/.."
OUT=build-out/coverage
rm -rf "$OUT"
mkdir -p "$OUT"

# An absolute source path, so gcov finds the sources from the output folder
g++ -std=c++17 -O0 --coverage -o "$OUT/test_logic" "$(pwd)/tests/test_logic.cpp"
"$OUT/test_logic" > "$OUT/tests.log" || { cat "$OUT/tests.log"; echo "::error::Unit tests failed"; exit 1; }

# gcov prints a summary per source file; keep the one for elpris_logic.h
# (the data file's name differs between GCC versions, so look it up)
GCDA=$(cd "$OUT" && find . -name '*.gcda' -print -quit)
REPORT=$(cd "$OUT" && gcov -b -c "$GCDA" 2>/dev/null)
SUMMARY=$(printf '%s\n' "$REPORT" | awk '
  /^File .*elpris_logic\.h.$/ { found = 1; next }
  found && /^Lines executed:/      { split($2, a, ":"); sub(/%/, "", a[2]); lines = a[2]; nlines = $4 }
  found && /^Taken at least once:/ { split($4, a, ":"); sub(/%/, "", a[2]); branches = a[2]; nbranches = $6; print lines, nlines, branches, nbranches; exit }
')
if [ -z "$SUMMARY" ]; then
  echo "::error::No coverage found for elpris_logic.h"
  exit 1
fi
read -r LINES NLINES BRANCHES NBRANCHES <<<"$SUMMARY"

# Lines that never ran, for the log and the summary page
UNCOVERED=$(awk -F: '$1 ~ /#####/ { gsub(/ /, "", $2); printf "%s ", $2 }' "$OUT/elpris_logic.h.gcov")

below() { awk -v a="$1" -v b="$2" 'BEGIN { exit !(a < b) }'; }
STATUS="✅ OK"
FAIL=0
if below "$LINES" "$MIN_LINES"; then STATUS="❌ lines below ${MIN_LINES} %"; FAIL=1; fi
if below "$BRANCHES" "$MIN_BRANCHES"; then STATUS="❌ branches below ${MIN_BRANCHES} %"; FAIL=1; fi

echo "Coverage of elpris_logic.h: lines ${LINES} % of ${NLINES}, branches ${BRANCHES} % of ${NBRANCHES}"
echo "Lines never run: ${UNCOVERED:-none}"

if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "### Test coverage of elpris_logic.h"
    echo ""
    echo "| | Covered | Limit |"
    echo "|---|---:|---:|"
    echo "| Lines | ${LINES} % of ${NLINES} | ${MIN_LINES} % |"
    echo "| Branches | ${BRANCHES} % of ${NBRANCHES} | ${MIN_BRANCHES} % |"
    echo ""
    echo "${STATUS}. Lines never run: ${UNCOVERED:-none}"
    echo ""
  } >> "$GITHUB_STEP_SUMMARY"
fi

if [ "$FAIL" -ne 0 ]; then
  echo "::error::Test coverage of elpris_logic.h is too low (${STATUS#❌ }). Add tests in tests/test_logic.cpp."
  exit 1
fi
