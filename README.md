# Elpris-LED

An ESP32 or ESP8266 and a WS2812B LED strip that show the day's Swedish spot electricity prices as colors, from green (cheap) to red (expensive). Prices come from [elprisetjustnu.se](https://www.elprisetjustnu.se).

## Getting started

You don't need to compile anything. Every release is built and tested automatically for both ESP32 and ESP8266, and there are three ways to get it onto your board:

1. **Install page (easiest):** open [flysen.github.io/elpris-led](https://flysen.github.io/elpris-led/) in Chrome or Edge on a computer, plug in the board with USB and click *Install on your board*. The page detects whether it's an ESP32 or an ESP8266 and installs the right firmware. Choose to erase the device on the first install.
2. **Download a release:** get the `-full.bin` file for your board from [Releases](https://github.com/flysen/elpris-led/releases) and flash it at address 0x0 with any ESP flashing tool, for example:
   ```bash
   pip install esptool
   esptool.py write_flash 0x0 elpris-led-<version>-esp32-full.bin     # ESP32
   esptool.py write_flash 0x0 elpris-led-<version>-esp8266-full.bin   # ESP8266
   ```
   Each release has a `SHA256SUMS` file if you want to verify the download.
3. **Build it yourself:** see [Building from source](#building-from-source). Only needed if you want to change the code.

If no USB port shows up, use a USB cable that carries data, not just power. On Windows and Mac the computer may need a driver for the board's USB chip, usually CP210x or CH340. On Linux, your user must be in the `dialout` group (`sudo usermod -a -G dialout $USER`, then log out and in).

### First start

1. The strip pulses blue and the lamp starts its own wifi, **Elpris-LED-Config**. Connect to it with your phone.
2. If no page opens, go to `http://192.168.4.1`. Choose your wifi and fill in price area (SE1–SE4), number of LEDs, night mode and brightness.
3. The lamp joins your network, fetches prices and starts showing colors.
4. Open **http://elpris-led.local** in a browser on the same network to change settings later. If that doesn't work (some Android phones and older Windows versions don't support `.local` names, and browser VPNs block them), look for **elpris-led-xxxxxx** in your router's list of devices and open its IP address. Set an **OTA password** and a **config portal password** right away; until then OTA is off and the portal is open.
5. Check that **Number of LEDs** matches your strip. If it's too high, part of the day ends up past the end of the strip.
6. If time runs the wrong way for how the strip is mounted, tick **Reverse direction** in the web UI. By default midnight is at the end where the strip's cable is connected.

### Finding the lamp on the network

| Where | Name |
|---|---|
| Browser | `http://elpris-led.local` |
| Router's device list | `elpris-led-xxxxxx`, where `xxxxxx` is the end of the board's MAC address |
| Arduino IDE network ports (OTA) | `elpris-led-xxxxxx`, once an OTA password is set |

With several lamps on the same network, only one of them answers to `elpris-led.local`. Use the router names and IP addresses for the others; the web UI of each lamp shows its own name and IP address. If your router has a saved name or a fixed IP for the board from an earlier project, it may show that name instead; rename the entry in the router.

## Reading the strip

- The strip runs from midnight today to midnight tonight, or to midnight tomorrow once tomorrow's prices are published (usually after lunch).
- **Green** is the cheapest and **red** the most expensive time in the period shown.
- The LED for **the current 15 minutes pulses towards white**.
- **Time that has passed is dimmed** (to 10 % by default), so the hours that are left, the ones worth planning around, stand out.

In the web UI you can turn the white pulse off, and set **Brightness of passed time** anywhere from 0 % (passed time off) to 100 % (not dimmed).

## Features

- Shows today from midnight to midnight along the strip, and tomorrow too once prices are published.
- "Now" is easy to see: a white pulse on the current 15 minutes, and dimmed past hours with adjustable brightness.
- The color scale is relative to the period shown, so even a cheap day has red hours.
- The direction can be reversed, so time reads left to right however the strip is mounted.
- Price area, number of LEDs (up to 300), direction, how "now" is shown, brightness of passed time, time zone, night mode hours and day/night brightness are set in a web UI at `http://elpris-led.local`.
- Shows up as `elpris-led-xxxxxx` in the router, so it's easy to find.
- Updates itself: new releases are found within six hours and installed automatically, for each board type.
- Few API calls; cached prices are used if fetching fails, with automatic retries. The web UI shows the reason when fetching fails.
- OTA updates and a config portal password, both set in the web UI. No passwords in the code.
- Handles daylight saving time correctly (23- and 25-hour days).

## Status colors

| What you see | Meaning |
|---|---|
| Whole strip pulses blue | Waiting for time or prices |
| Whole strip pulses purple | No prices for now and fetching fails |
| First three LEDs (from midnight) blink magenta | Fetching fails, cached prices still shown |
| Dim cyan dots | Installing an update, the lamp restarts when done |

Prices are spot prices excluding VAT, grid fees and energy tax.

## Hardware

**Board**, one of:

| Board | Examples | Strip data pin | Notes |
|---|---|---|---|
| **ESP32** (recommended) | ESP32 DevKit, ESP32-WROOM-32, NodeMCU-32 | GPIO16 (marked D16 or RX2) | 4 MB flash. Not ESP32-C3, -S2 or -S3. More memory, so the most robust choice. |
| **ESP8266** | NodeMCU, Wemos D1 mini | GPIO2 (marked D4) | Must have 4 MB flash (not ESP-01). Same pin as WLED uses. |

**Also needed:**

- WS2812B strip, 5 V, up to 300 LEDs, and a 5 V supply sized for it (up to 60 mA per LED at full white)
- 330–470 Ω resistor in the data line, 1000 µF capacitor across +5 V/GND at the strip input

```
5 V supply          ESP32 / ESP8266            WS2812B strip (DIN end)
  +5V ─────┬──────── 5V / VIN
           └────────────────────────────────── +5V
  GND ─────┬──────── GND
           └────────────────────────────────── GND
                     data pin ── 330 Ω ────── DIN
                     (ESP32: GPIO16 / D16, ESP8266: GPIO2 / D4)
```

The board and the strip must share ground. Connect the strip at the end where the arrows point away from you (DIN). The strip can be mounted with that end on either side; use *Reverse direction* in the web UI to make time run left to right. Don't power the board from USB and the 5 V supply at the same time unless your board allows it: install over USB first, then connect the supply.

## Updating

- **Automatic (default):** the lamp checks for a new release two minutes after start and then every six hours. When it finds one, it restarts, installs the new firmware for its own board right after reconnecting to wifi (before anything else uses memory), and restarts again. This takes about a minute, with dim cyan dots on the strip. Wifi and settings are kept. If the download fails, the lamp starts normally and tries again at the next check.
- **Check for updates now:** the button in the lamp's web UI does the same thing right away. The page reloads by itself while it checks and installs, and shows the current version, board and latest release. You can also turn automatic updates off there.
- **Install page:** run it again and choose *not* to erase. Wifi and settings are kept.
- **Arduino OTA:** if you've set an OTA password, the lamp shows up as `elpris-led-xxxxxx` among the network ports in the Arduino IDE. You can also upload `elpris-led-<version>-<board>-ota.bin` from [Releases](https://github.com/flysen/elpris-led/releases) with `espota.py`.

Builds you compile yourself, and CI builds, report version `dev` and never update automatically, so your own changes aren't overwritten. Flash a release to get back on automatic updates.

## Building from source

Only needed if you want to change the code. One sketch, `elpris_led/`, builds for both boards; the few differences (includes, data pin, settings storage, mDNS, OTA and price cache size) are in `elpris_led/platform.h`. With [arduino-cli](https://arduino.github.io/arduino-cli/) installed:

```bash
bash scripts/arduino-deps.sh                # ESP32 and ESP8266 cores and libraries at the pinned versions
bash scripts/build-firmware.sh dev esp32    # build and check one board into build-out/<board> and dist/
bash scripts/build-firmware.sh dev esp8266
bash scripts/package.sh dev                 # install page with manifest.json in site/, and dist/SHA256SUMS
```

To upload directly to a connected board:

```bash
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs -p /dev/ttyUSB0 --input-dir build-out/esp32 elpris_led
arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2:eesz=4M1M -p /dev/ttyUSB0 --input-dir build-out/esp8266 elpris_led
```

In the Arduino IDE: add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` and `https://arduino.esp8266.com/stable/package_esp8266com_index.json` under *Preferences*, install *esp32* and *esp8266* in the Boards Manager and FastLED, ArduinoJson and WiFiManager (by tzapu) in the Library Manager, then open `elpris_led/elpris_led.ino`. Board settings:

- **ESP32:** board *ESP32 Dev Module*, *Partition Scheme* **Minimal SPIFFS (1.9MB APP with OTA)**.
- **ESP8266:** board *NodeMCU 1.0 (ESP-12E Module)*, *Flash Size* **4MB (FS:1MB OTA:~1019KB)**.

Use the versions listed in `scripts/arduino-deps.sh` to get the same result as a release.

The time, slot, LED order, past/now/future, dimming, night mode and version logic is in `elpris_led/elpris_logic.h`, shared by both boards, and is tested on a regular computer:

```bash
make test            # unit tests
make test-sanitize   # the same tests with AddressSanitizer and UndefinedBehaviorSanitizer
```

### Continuous integration

Every pull request and every push to `main` runs:

- **tests:** the unit tests, normally and with sanitizers.
- **lint:** shellcheck on the build scripts.
- **firmware:** the same build and packaging as a release, for both ESP32 and ESP8266, with pinned dependencies. For each board it checks that the app uses less than 95 % of its space (warning above 85 %), that the version is in the binary, and that the install page manifest is valid. The sizes are shown on the run's summary page, and the firmware is attached to the run for 14 days so you can try a pull request on your own lamp before releasing it.

Dependabot opens pull requests when the GitHub Actions used here get new versions. The core and library versions are bumped by hand in `scripts/arduino-deps.sh`.

## Releases (for maintainers)

Pushing a version tag checks the tag, runs the tests, builds the firmware for both boards with that version number, publishes a release with `-full.bin` and `-ota.bin` per board plus `SHA256SUMS`, and updates the install page. Lamps in the field pick up the new version from the install page within six hours.

```bash
git tag v2.1
git push origin v2.1
```

Or in the browser: *Releases → Draft a new release*, click *Tag*, type a new tag, click *Create new tag*, choose `main` and publish.

The release stops before anything is published if:

- the tag isn't `v2.1`, `v2.1.3` or `v2.1-rc1`,
- the tagged commit isn't on `main`,
- a stable tag isn't higher than the previous stable release (lamps would ignore it),
- any test or build check fails, for either board.

**Release candidates** (`v2.1-rc1`) are published as pre-releases only. They don't update the install page and lamps never install them automatically. Flash the `-full.bin` on your own lamp, and when it works, tag `v2.1` on the same commit. A lamp running `v2.1-rc1` moves on to `v2.1` by itself.

**Flash layout:** ESP32 uses the *Minimal SPIFFS* partition scheme (two 1.9 MB app slots) and ESP8266 the *4MB (FS:1MB)* layout. Automatic updates only replace the app, never the flash layout, so these must not change in a release. If they ever have to, lamps need a USB install from the install page.

**Install page paths:** lamps download `firmware/<board>/elpris-led.bin`. Lamps on v2.0 (ESP32 only) use `firmware/elpris-led.bin`, which the build keeps as a copy of the ESP32 app.

One-time repository setup before the first release:

1. *Settings → Pages*: set *Source* to *GitHub Actions*.
2. *Settings → Environments → github-pages*: allow tags matching `v*` under deployment branches and tags. Otherwise the deploy is blocked because it runs from a tag.
3. *Settings → Rules → Rulesets*: protect `main` and require the CI checks *tests*, *lint* and *firmware* to pass before merging.

## License

See [LICENSE](LICENSE).
