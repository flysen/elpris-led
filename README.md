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
   Each release has a `SHA256SUMS` file if you want to verify the download.
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
- Updates itself: new releases are found and installed automatically.
- Few API calls; cached prices are used if fetching fails, with automatic retries.
- OTA updates and a config portal password, both set in the web UI. No passwords in the code.
- Handles daylight saving time correctly (23- and 25-hour days).

## Status colors

| What you see | Meaning |
|---|---|
| Whole strip pulses blue | Waiting for time or prices |
| Whole strip pulses purple | No prices for now and fetching fails |
| First three LEDs blink magenta | Fetching fails, cached prices still shown |
| Dim cyan dots | Installing an update, the lamp restarts when done |

Prices are spot prices excluding VAT, grid fees and energy tax.

## Hardware

- ESP32 dev board (classic ESP32 with 4 MB flash, e.g. DevKitC / WROOM-32; C3, S2 and S3 are not supported by the prebuilt firmware)
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

- **Automatic (default):** the lamp checks for a new release two minutes after start and then once a day, and installs it by itself. Wifi and settings are kept. You can turn this off, or press *Check for updates now*, in the lamp's web UI. The current and latest version are shown there too.
- **Install page:** run it again and choose *not* to erase. Wifi and settings are kept.
- **Arduino OTA:** if you've set an OTA password, upload `elpris-led-<version>-esp32-ota.bin` from [Releases](https://github.com/flysen/elpris-led/releases) with the Arduino IDE or `espota.py`.

Builds you compile yourself, and CI builds, report version `dev` and never update automatically, so your own changes aren't overwritten. Flash a release to get back on automatic updates.

## Building from source

Only needed if you want to change the code, for example to use another data pin (`DATA_PIN` in `elpris_led/elpris_led.ino`). With [arduino-cli](https://arduino.github.io/arduino-cli/) installed:

```bash
bash scripts/arduino-deps.sh          # ESP32 core and libraries at the pinned versions
bash scripts/build-firmware.sh dev    # build, check and package into dist/ and site/
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs -p /dev/ttyUSB0 --input-dir build-out elpris_led
```

In the Arduino IDE: add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` under *Preferences*, install *esp32* in the Boards Manager and FastLED, ArduinoJson and WiFiManager (by tzapu) in the Library Manager, then open `elpris_led/elpris_led.ino`. Choose the board *ESP32 Dev Module* and under *Tools → Partition Scheme* choose **Minimal SPIFFS (1.9MB APP with OTA)**; the firmware doesn't fit the default scheme with room for OTA. Use the versions listed in `scripts/arduino-deps.sh` to get the same result as a release.

The time, slot, night mode and version logic is in `elpris_led/elpris_logic.h` and is tested on a regular computer:

```bash
make test            # unit tests
make test-sanitize   # the same tests with AddressSanitizer and UndefinedBehaviorSanitizer
```

### Continuous integration

Every pull request and every push to `main` runs:

- **tests:** the unit tests, normally and with sanitizers.
- **lint:** shellcheck on the build scripts.
- **firmware:** the same build and packaging as a release, with pinned dependencies. It checks that the app fits in the OTA partition, that the version is in the binary and that the install page manifest is valid. The firmware is attached to the run for 14 days, so you can try a pull request on your own lamp before releasing it.

Dependabot opens pull requests when the GitHub Actions used here get new versions. The ESP32 core and library versions are bumped by hand in `scripts/arduino-deps.sh`.

## Releases (for maintainers)

Pushing a version tag checks the tag, runs the tests, builds the firmware with that version number, publishes a release with `-full.bin`, `-ota.bin` and `SHA256SUMS`, and updates the install page. Lamps in the field pick up the new version from the install page within a day.

```bash
git tag v2.1
git push origin v2.1
```

Or in the browser: *Releases → Draft a new release*, type a new tag, choose `main` and publish.

The release stops before anything is published if:

- the tag isn't `v2.1`, `v2.1.3` or `v2.1-rc1`,
- the tagged commit isn't on `main`,
- a stable tag isn't higher than the previous stable release (lamps would ignore it),
- any test or build check fails.

**Release candidates** (`v2.1-rc1`) are published as pre-releases only. They don't update the install page and lamps never install them automatically. Flash the `-ota.bin` or `-full.bin` on your own lamp, and when it works, tag `v2.1` on the same commit. A lamp running `v2.1-rc1` moves on to `v2.1` by itself.

**Partition table:** the firmware uses the *Minimal SPIFFS* scheme (two 1.9 MB app slots). Automatic updates only replace the app, never the partition table, so this must not change in a release. If it ever has to, lamps need a USB install from the install page.

One-time repository setup before the first release:

1. *Settings → Pages*: set *Source* to *GitHub Actions*.
2. *Settings → Environments → github-pages*: allow tags matching `v*` under deployment branches and tags. Otherwise the deploy is blocked because it runs from a tag.
3. *Settings → Rules → Rulesets*: protect `main` and require the CI checks *tests*, *lint* and *firmware* to pass before merging.

## License

See [LICENSE](LICENSE).
