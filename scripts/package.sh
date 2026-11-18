#!/usr/bin/env bash
# Finishes the install page and release files after every board has been built.
# Usage: scripts/package.sh <version>
# Output: site/ (install page with manifest.json) and dist/SHA256SUMS
set -euo pipefail

VERSION="${1:?usage: $0 <version>}"
cd "$(dirname "$0")/.."

test -s "site/firmware/esp32/elpris-led.bin" || { echo "::error::No esp32 build found. Run scripts/build-firmware.sh $VERSION esp32 first."; exit 1; }

cp -r docs/. site/

# ESP32 only from v5.0. The installer reads the connected board's chip and refuses other chips.
cat > site/manifest.json << EOF
{
  "name": "Elpris-LED",
  "version": "${VERSION}",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        { "path": "firmware/esp32/bootloader.bin", "offset": 4096 },
        { "path": "firmware/esp32/partitions.bin", "offset": 32768 },
        { "path": "firmware/esp32/boot_app0.bin", "offset": 57344 },
        { "path": "firmware/esp32/elpris-led.bin", "offset": 65536 }
      ]
    }
  ]
}
EOF
python3 -m json.tool site/manifest.json > /dev/null || { echo "::error::manifest.json is not valid JSON"; exit 1; }

(cd dist && sha256sum ./*.bin > SHA256SUMS)
echo "Packaged Elpris-LED ${VERSION}:"
cat dist/SHA256SUMS
