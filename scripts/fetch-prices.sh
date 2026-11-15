#!/usr/bin/env bash
# Fetches the spot prices for SE1-SE4, from three days ago to tomorrow, from elprisetjustnu.se and writes
# them as one compact JSON file to stdout, for the emulator. Run every hour by .github/workflows/prices.yml.
#
# Output: {"updated": "...", "source": "...", "areas": {"SE3": {"2026-11-15": [1.31, 1.22, ...], ...}, ...}}
# Each day is a list of SEK/kWh excl. VAT, one value per price period from local midnight, in order
# (96 quarters on an ordinary day). Tomorrow is left out until it is published, usually from 13:00.
set -euo pipefail

export TZ=Europe/Stockholm
readonly AREAS=(SE1 SE2 SE3 SE4)
readonly DAYS_BACK=3
readonly USER_AGENT="Elpris-LED emulator (+https://github.com/flysen/elpris-led)"

out='{}'
for area in "${AREAS[@]}"; do
  for offset in $(seq "-$DAYS_BACK" 1); do
    day=$(date -d "$offset day" +%F)
    url="https://www.elprisetjustnu.se/api/v1/prices/$(date -d "$day" +%Y/%m-%d)_${area}.json"
    if json=$(curl -fsS --retry 3 --retry-delay 5 -A "$USER_AGENT" "$url"); then
      prices=$(jq -c 'map(.SEK_per_kWh)' <<<"$json")
      out=$(jq -c --arg a "$area" --arg d "$day" --argjson p "$prices" '.[$a][$d] = $p' <<<"$out")
    elif [ "$offset" -lt 1 ]; then
      # Today and the past days must exist; only tomorrow may be missing
      echo "Could not fetch $area $day from $url" >&2
      exit 1
    fi
  done
done

jq -c --arg u "$(date -Iseconds)" '{updated: $u, source: "https://www.elprisetjustnu.se", areas: .}' <<<"$out"
