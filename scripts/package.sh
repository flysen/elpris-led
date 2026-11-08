#!/usr/bin/env bash
# Finishes the install page and release files after every board has been built.
# Usage: scripts/package.sh <version>
# Output: site/ (install page with manifest.json) and dist/SHA256SUMS
set -euo pipefail

VERSION="${1:?usage: $0 <version>}"
cd "$(dirname "$0")/.."

for b in esp32 esp8266; do
  test -s "site/firmware/$b/elpris-led.bin" || { echo "::error::No $b build found. Run scripts/build-firmware.sh $VERSION $b first."; exit 1; }
done

cp -r docs/. site/

# The installer reads the connected board's chip and picks the matching build
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
    },
    {
      "chipFamily": "ESP8266",
      "parts": [
        { "path": "firmware/esp8266/elpris-led.bin", "offset": 0 }
      ]
    }
  ]
}
EOF
python3 -m json.tool site/manifest.json > /dev/null || { echo "::error::manifest.json is not valid JSON"; exit 1; }

(cd dist && sha256sum ./*.bin > SHA256SUMS)
echo "Packaged Elpris-LED ${VERSION}:"
cat dist/SHA256SUMS
