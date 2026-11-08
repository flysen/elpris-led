#!/usr/bin/env bash
# Builds the firmware for one board, checks it and puts the release and install page files in place.
# Usage: scripts/build-firmware.sh <version> <board>
#   version: a release tag like v2.1, or "dev" for CI builds
#   board:   esp32 or esp8266
# Run it once per board, then scripts/package.sh <version> to write manifest.json and SHA256SUMS.
set -euo pipefail

VERSION="${1:?usage: $0 <version> <board>}"
BOARD="${2:?usage: $0 <version> <board>}"

# The flash layout must never change in a release: automatic updates only replace the app,
# so a lamp in the field keeps the layout it was installed with.
case "$BOARD" in
  esp32)
    # Minimal SPIFFS: two 1.9 MB app slots for OTA. The NVS partition with settings is at the default place.
    FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
    APP_MAX=1966080 ;;
  esp8266)
    # 4 MB flash: up to 1 MB app, 1 MB LittleFS for settings, the rest is room for OTA.
    FQBN="esp8266:esp8266:nodemcuv2:eesz=4M1M"
    APP_MAX=1044464 ;;
  *)
    echo "::error::Unknown board $BOARD (use esp32 or esp8266)"; exit 1 ;;
esac
SIZE_WARN_PCT=85  # warn above this share of the app space
SIZE_FAIL_PCT=95  # fail above this share, to keep a safety margin before updates break

cd "$(dirname "$0")/.."
OUT="build-out/$BOARD"
SITE_FW="site/firmware/$BOARD"
rm -rf "$OUT" "$SITE_FW"
rm -f dist/elpris-led-*-"$BOARD"-*.bin
mkdir -p "$OUT" "$SITE_FW" dist

# Version shown in the web UI and compared by the automatic updater
printf '#define FW_VERSION "%s"\n' "$VERSION" > elpris_led/version.h

arduino-cli compile --fqbn "$FQBN" --warnings default --output-dir "$OUT" elpris_led

B="$OUT/elpris_led.ino"
test -s "$B.bin" || { echo "::error::Missing build output: $B.bin"; exit 1; }

# The app must fit with margin, otherwise automatic updates will soon break
SIZE=$(stat -c %s "$B.bin")
PCT=$((SIZE * 100 / APP_MAX))
FREE=$((APP_MAX - SIZE))
if [ "$PCT" -ge "$SIZE_FAIL_PCT" ]; then
  STATUS="❌ over the ${SIZE_FAIL_PCT} % limit"
elif [ "$PCT" -ge "$SIZE_WARN_PCT" ]; then
  STATUS="⚠️ above ${SIZE_WARN_PCT} %"
else
  STATUS="✅ OK"
fi
echo "App size ($BOARD): $SIZE of $APP_MAX bytes ($PCT %), $FREE bytes free"

# Show the size on the run's summary page, so nobody has to dig in the log
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "### Firmware ${VERSION} (${BOARD})"
    echo ""
    echo "| App size | App space | Used | Free | Status |"
    echo "|---:|---:|---:|---:|---|"
    echo "| $SIZE bytes | $APP_MAX bytes | $PCT % | $FREE bytes | $STATUS |"
    echo ""
    echo "Warning above ${SIZE_WARN_PCT} %, failure at ${SIZE_FAIL_PCT} % or more."
    echo ""
  } >> "$GITHUB_STEP_SUMMARY"
fi

if [ "$PCT" -ge "$SIZE_FAIL_PCT" ]; then
  echo "::error::The $BOARD app uses $PCT % of its space (limit ${SIZE_FAIL_PCT} %). Make it smaller before merging."
  exit 1
fi
if [ "$PCT" -ge "$SIZE_WARN_PCT" ]; then
  echo "::warning::The $BOARD app uses $PCT % of its space. The build fails at ${SIZE_FAIL_PCT} %."
fi

# The version must be inside the binary, or lamps would keep installing the same release
grep -q -a -F "$VERSION" "$B.bin" || { echo "::error::Version $VERSION not found in the $BOARD binary"; exit 1; }

FULL="dist/elpris-led-${VERSION}-${BOARD}-full.bin"
OTA="dist/elpris-led-${VERSION}-${BOARD}-ota.bin"

if [ "$BOARD" = "esp32" ]; then
  for f in "$B.bootloader.bin" "$B.partitions.bin"; do
    test -s "$f" || { echo "::error::Missing build output: $f"; exit 1; }
  done
  BOOT_APP0=$(find ~/.arduino15/packages/esp32/hardware -name boot_app0.bin -print -quit)
  test -n "$BOOT_APP0" || { echo "::error::boot_app0.bin not found"; exit 1; }

  # Install page: separate parts, so wifi and settings are kept on update
  cp "$B.bootloader.bin" "$SITE_FW/bootloader.bin"
  cp "$B.partitions.bin" "$SITE_FW/partitions.bin"
  cp "$BOOT_APP0"        "$SITE_FW/boot_app0.bin"
  cp "$B.bin"            "$SITE_FW/elpris-led.bin"
  # Lamps running v2.0 download firmware/elpris-led.bin; keep that path working for them
  cp "$B.bin"            site/firmware/elpris-led.bin

  # Full image for esptool (flash at 0x0)
  MERGED="$B.merged.bin"
  if [ ! -f "$MERGED" ]; then
    ESPTOOL=$(find ~/.arduino15/packages/esp32/tools -name esptool -type f -perm -u+x -print -quit)
    test -n "$ESPTOOL" || { echo "::error::esptool not found"; exit 1; }
    "$ESPTOOL" --chip esp32 merge_bin -o "$MERGED" \
      0x1000 "$B.bootloader.bin" \
      0x8000 "$B.partitions.bin" \
      0xe000 "$BOOT_APP0" \
      0x10000 "$B.bin"
  fi
  MERGED_SIZE=$(stat -c %s "$MERGED")
  if [ "$MERGED_SIZE" -lt $((0x10000 + SIZE)) ]; then
    echo "::error::The full image is smaller than expected ($MERGED_SIZE bytes)"; exit 1
  fi
  cp "$MERGED" "$FULL"
else
  # On ESP8266 the app image includes the boot loader and is flashed at 0x0
  cp "$B.bin" "$SITE_FW/elpris-led.bin"
  cp "$B.bin" "$FULL"
fi
cp "$B.bin" "$OTA"

echo "Built Elpris-LED ${VERSION} for ${BOARD}"
