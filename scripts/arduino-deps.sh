#!/usr/bin/env bash
# Installs the ESP32 and ESP8266 cores and the libraries at pinned versions,
# so every build uses exactly the same code.
# Bump these on purpose, in their own pull request, and let CI build it before the next release.
set -euo pipefail

ESP32_CORE="3.3.12"
ESP8266_CORE="3.1.2"
LIBS=("FastLED@3.10.8" "ArduinoJson@7.4.3" "WiFiManager@2.0.17")
BOARD_URLS="https://espressif.github.io/arduino-esp32/package_esp32_index.json,https://arduino.esp8266.com/stable/package_esp8266com_index.json"

# Downloads from GitHub and the board package servers sometimes time out; try a few times
retry() {
  local attempt
  for attempt in 1 2 3; do
    if "$@"; then return 0; fi
    echo "::warning::Attempt $attempt failed: $*"
    sleep $((attempt * 20))
  done
  echo "::error::Giving up after 3 attempts: $*"
  return 1
}

retry arduino-cli core update-index --additional-urls "$BOARD_URLS"
retry arduino-cli core install "esp32:esp32@${ESP32_CORE}" --additional-urls "$BOARD_URLS"
retry arduino-cli core install "esp8266:esp8266@${ESP8266_CORE}" --additional-urls "$BOARD_URLS"
retry arduino-cli lib install "${LIBS[@]}"
