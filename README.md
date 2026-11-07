# Elpris-LED

An ESP32 and a WS2812B LED strip that show the day's Swedish spot electricity prices as colors, from green (cheap) to red (expensive). Prices come from [elprisetjustnu.se](https://www.elprisetjustnu.se).

## Getting started

You don't need to compile anything. Every release is built and tested automatically, and there are three ways to get it onto your ESP32:

1. **Install page (easiest):** open [flysen.github.io/elpris-led](https://flysen.github.io/elpris-led/) in Chrome or Edge on a computer, plug in the ESP32 with USB and click *Install on ESP32*. Choose to erase the device on the first install.
2. **Download a release:** get `elpris-led-<version>-esp32-full.bin` from [Releases](https://github.com/flysen/elpris-led/releases) and flash it at address 0x0 with any ESP flashing tool, for example:
   ```bash
   pip install esptool
   esptool.py --chip esp32 write_flash 0x0 elpris-led-<version>-esp32-full.bin
   ```
3. **Build it yourself:** see [Building from source](#building-from-source). Only needed if you want to change the code.

If no USB port shows up, the computer needs a driver for the board's USB chip, usually CP210x or CH340. Some USB cables only charge; use one that carries data.

### First start

1. The strip pulses blue and the lamp starts its own wifi, **Elpris-LED-Config**. Connect to it with your phone.
2. If no page opens, go to `http://192.168.4.1`. Choose your wifi and fill in price area (SE1–SE4), number of LEDs, night mode and brightness.
3. The lamp joins your network, fetches prices and starts showing colors.
4. Find the lamp's IP address in your router and open it in a browser to change settings later. Set an **OTA password** and a **config portal password** right away; until then OTA is off and the portal is open.

## Features

- Shows today from midnight to midnight along the strip, and tomorrow too once prices are published (usually after lunch).
- The LED for the current 15 minutes pulses.
- The color scale is relative to the period shown, so even a cheap day has red hours.
- Price area, number of LEDs (up to 300), time zone, night mode hours and day/night brightness are set in a web UI.
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
- WS2812B strip, 5 V, up to 300 LEDs, and a 5 V supply sized for it (up to 60 mA per LED at full white)
- 330–470 Ω resistor in the data line, 1000 µF capacitor across +5 V/GND at the strip input

```
5 V supply          ESP32                 WS2812B strip (DIN end)
  +5V ─────┬──────── 5V / VIN
           └───────────────────────────── +5V
  GND ─────┬──────── GND
           └───────────────────────────── GND
                     GPIO16 ── 330 Ω ──── DIN
```

The ESP32 and the strip must share ground. Connect the strip at the end where the arrows point away from you (DIN). Don't power the ESP32 from USB and the 5 V supply at the same time unless your board allows it: install over USB first, then connect the supply.

## Updating

- **Install page:** run it again and choose *not* to erase. Wifi and settings are kept.
- **Over the air:** if you've set an OTA password, upload `elpris-led-<version>-esp32-ota.bin` from [Releases](https://github.com/flysen/elpris-led/releases) with the Arduino IDE or `espota.py`.

## Building from source

Only needed if you want to change the code, for example to use another data pin (`DATA_PIN` in `elpris_led/elpris_led.ino`). With [arduino-cli](https://arduino.github.io/arduino-cli/):

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install FastLED ArduinoJson WiFiManager
arduino-cli compile --fqbn esp32:esp32:esp32 elpris_led
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 elpris_led
```

In the Arduino IDE: add the board URL above under *Preferences*, install *esp32* in the Boards Manager and FastLED, ArduinoJson and WiFiManager (by tzapu) in the Library Manager, then open `elpris_led/elpris_led.ino`.

The time, slot and night mode logic is in `elpris_led/elpris_logic.h` and is tested on a regular computer with:

```bash
make test
```

Every push and pull request is compiled and tested in GitHub Actions.

## Releases (for maintainers)

Pushing a version tag runs the tests, builds the firmware, publishes a release with `-full.bin` and `-ota.bin`, and updates the install page:

```bash
git tag v2.0
git push origin v2.0
```

One-time repository setup before the first release:

1. *Settings → Pages*: set *Source* to *GitHub Actions*.
2. *Settings → Environments → github-pages*: allow tags matching `v*` under deployment branches and tags. Otherwise the deploy is blocked because it runs from a tag.

## License

See [LICENSE](LICENSE).
