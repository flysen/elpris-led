#!/usr/bin/env bash
# Builds the firmware and packages it for a release and the install page, with sanity checks.
# Usage: scripts/build-firmware.sh <version>   (a release tag like v2.1, or "dev" for CI builds)
# Output: dist/ (release files + SHA256SUMS) and site/ (install page with firmware and manifest.json)
set -euo pipefail

VERSION="${1:?usage: $0 <version>}"
# Minimal SPIFFS partition scheme: two 1.9 MB app slots for OTA (the default only has 1.25 MB).
# The NVS partition with wifi and settings is at the same place in both schemes.
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
OUT="build-out"
APP_MAX=1966080   # app partition size in the min_spiffs partition table (one OTA slot)
SIZE_WARN_PCT=85  # warn above this share of the OTA slot
SIZE_FAIL_PCT=95  # fail above this share, to keep a safety margin before updates break

cd "$(dirname "$0")/.."
rm -rf "$OUT" site dist
mkdir -p site/firmware dist

# Version shown in the web UI and compared by the automatic updater
printf '#define FW_VERSION "%s"\n' "$VERSION" > elpris_led/version.h

arduino-cli compile --fqbn "$FQBN" --warnings default --output-dir "$OUT" elpris_led

B="$OUT/elpris_led.ino"
for f in "$B.bin" "$B.bootloader.bin" "$B.partitions.bin"; do
  test -s "$f" || { echo "::error::Missing build output: $f"; exit 1; }
done
BOOT_APP0=$(find ~/.arduino15/packages/esp32/hardware -name boot_app0.bin -print -quit)
test -n "$BOOT_APP0" || { echo "::error::boot_app0.bin not found"; exit 1; }

# The app must fit in an OTA slot with margin, otherwise automatic updates will soon break
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
echo "App size: $SIZE of $APP_MAX bytes ($PCT %), $FREE bytes free"

# Show the size on the run's summary page, so nobody has to dig in the log
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "### Firmware ${VERSION}"
    echo ""
    echo "| App size | OTA slot | Used | Free | Status |"
    echo "|---:|---:|---:|---:|---|"
    echo "| $SIZE bytes | $APP_MAX bytes | $PCT % | $FREE bytes | $STATUS |"
    echo ""
    echo "Warning above ${SIZE_WARN_PCT} %, failure at ${SIZE_FAIL_PCT} % or more."
  } >> "$GITHUB_STEP_SUMMARY"
fi

if [ "$PCT" -ge "$SIZE_FAIL_PCT" ]; then
  echo "::error::The app uses $PCT % of the OTA partition (limit ${SIZE_FAIL_PCT} %). Make it smaller before merging."
  exit 1
fi
if [ "$PCT" -ge "$SIZE_WARN_PCT" ]; then
  echo "::warning::The app uses $PCT % of the OTA partition. The build fails at ${SIZE_FAIL_PCT} %."
fi

# The version must be inside the binary, or lamps would keep installing the same release
grep -q -a -F "$VERSION" "$B.bin" || { echo "::error::Version $VERSION not found in the app binary"; exit 1; }

# Install page: separate parts, so wifi and settings are kept on update.
# firmware/elpris-led.bin is also what lamps download for automatic updates.
cp -r docs/. site/
cp "$B.bootloader.bin" site/firmware/bootloader.bin
cp "$B.partitions.bin" site/firmware/partitions.bin
cp "$BOOT_APP0"        site/firmware/boot_app0.bin
cp "$B.bin"            site/firmware/elpris-led.bin

cat > site/manifest.json << EOF
{
  "name": "Elpris-LED",
  "version": "${VERSION}",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        { "path": "firmware/bootloader.bin", "offset": 4096 },
        { "path": "firmware/partitions.bin", "offset": 32768 },
        { "path": "firmware/boot_app0.bin", "offset": 57344 },
        { "path": "firmware/elpris-led.bin", "offset": 65536 }
      ]
    }
  ]
}
EOF
python3 -m json.tool site/manifest.json > /dev/null || { echo "::error::manifest.json is not valid JSON"; exit 1; }

# Release files: a full image for esptool (flash at 0x0) and a plain app file for OTA
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
cp "$MERGED" "dist/elpris-led-${VERSION}-esp32-full.bin"
cp "$B.bin"  "dist/elpris-led-${VERSION}-esp32-ota.bin"
(cd dist && sha256sum ./*.bin > SHA256SUMS)

echo "Built Elpris-LED ${VERSION}:"
cat dist/SHA256SUMS
