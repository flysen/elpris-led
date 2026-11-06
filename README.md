# Elpris-LED

An ESP32 and a WS2812B LED strip that show the day's Swedish spot electricity prices as colors, from green (cheap) to red (expensive). Prices come from [elprisetjustnu.se](https://www.elprisetjustnu.se).

**Install from the browser: [flysen.github.io/elpris-led](https://flysen.github.io/elpris-led/)** (Chrome or Edge, no compiling needed)

## Features

- Shows today from midnight to midnight along the strip, and tomorrow too once prices are published (usually after lunch).
- The LED for the current 15 minutes pulses.
- The color scale is relative to the period shown, so even a cheap day has red hours.
- Price area SE1–SE4, number of LEDs (up to 300), time zone, night mode hours and day/night brightness are set in a web UI.
- First-time setup through its own wifi, **Elpris-LED-Config**, at `http://192.168.4.1`.
- Few API calls; cached prices are used if fetching fails, with automatic retries.
- OTA updates and a config portal password, both set in the web UI. No passwords in the code.
- Handles daylight saving time correctly (23- and 25-hour days).

## Status colors

| What you see | Meaning |
|---|---|
| Whole strip pulses blue | Waiting for time or prices |
| Whole strip pulses purple | No prices for now and fetching fails |
| First three LEDs blink magenta | Fetching fails, cached prices still shown |

Prices are spot prices excluding VAT, grid fees and energy tax.

## Hardware

- ESP32 dev board (classic ESP32, e.g. DevKitC / WROOM-32; C3, S2 and S3 are not supported by the prebuilt firmware)
- WS2812B strip, 5 V, up to 300 LEDs, and a 5 V supply sized for it
- 330–470 Ω resistor in the data line, 1000 µF capacitor across +5 V/GND at the strip input

```
5 V supply          ESP32                 WS2812B strip (DIN end)
  +5V ─────┬──────── 5V / VIN
           └───────────────────────────── +5V
  GND ─────┬──────── GND
           └───────────────────────────── GND
                     GPIO16 ── 330 Ω ──── DIN
```

The ESP32 and the strip must share ground. Don't power the ESP32 from USB and the 5 V supply at the same time unless your board allows it.

## Updating

Use the install page again and choose *not* to erase to keep wifi and settings, or upload `elpris-led-<version>-esp32-ota.bin` from [Releases](https://github.com/flysen/elpris-led/releases) over OTA.

## Building

Build with [arduino-cli](https://arduino.github.io/arduino-cli/) or the Arduino IDE (libraries: FastLED, ArduinoJson, WiFiManager). The data pin is `DATA_PIN` in `elpris_led/elpris_led.ino`. Run the logic tests with `make test`.

Pushing a tag like `v2.1` builds the firmware, creates a release and updates the install page.

## License

See [LICENSE](LICENSE).
