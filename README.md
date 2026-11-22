# Elpris-LED

An ESP32 and a WS2812B LED strip that show the day's Swedish spot electricity prices as colors: green when it's a good time to use power, yellow when it doesn't matter, red when it's expensive.

Elpriser tillhandahålls av [Elpriset just nu.se](https://www.elprisetjustnu.se).

What's new in each version: see [CHANGELOG.md](CHANGELOG.md). Try it without a lamp, on today's real prices, in the [emulator](https://flysen.github.io/elpris-led/emulator/).

## Getting started

You don't need to compile anything. Every release is built and tested automatically, and there are three ways to get it onto your ESP32:

1. **Install page (easiest):** open [flysen.github.io/elpris-led](https://flysen.github.io/elpris-led/) in Chrome or Edge on a computer, plug in the board with USB and click *Install on your board*. Choose to erase the device on the first install. Some ESP32 boards need a press on their **EN/RST** button after installing before they start.
2. **Download a release:** get the `-full.bin` file for your board from [Releases](https://github.com/flysen/elpris-led/releases) and flash it at address 0x0 with any ESP flashing tool, for example:
   ```bash
   pip install esptool
   esptool.py write_flash 0x0 elpris-led-<version>-esp32-full.bin
   ```
   Each release has a `SHA256SUMS` file if you want to verify the download.
3. **Build it yourself:** see [Building from source](#building-from-source). Only needed if you want to change the code.

If no USB port shows up, use a USB cable that carries data, not just power. On Windows and Mac the computer may need a driver for the board's USB chip, usually CP210x or CH340. On Linux, your user must be in the `dialout` group (`sudo usermod -a -G dialout $USER`, then log out and in).

### First start

1. The strip pulses blue and the lamp starts its own wifi, **Elpris-LED-Config**; the board's blue LED blinks fast. Connect to it with your phone.
2. If no page opens, go to `http://192.168.4.1`. Choose your wifi and fill in price area (SE1–SE4), number of LEDs, night mode and brightness.
3. The lamp joins your network, fetches prices and starts showing colors. The board's blue LED then blinks the end of the lamp's IP address three times (see [The board's blue LED](#the-boards-blue-led)).
4. Open **http://elpris-led.local** in a browser on the same network to change settings later. If that doesn't work (some Android phones and older Windows versions don't support `.local` names, and browser VPNs block them), look for **elpris-led-xxxxxx** in your router's list of devices and open its IP address. Set an **OTA password** and a **config portal password** right away; until then OTA is off and the portal is open.
5. Check that **Number of LEDs** matches your strip. If it's too high, part of the day ends up past the end of the strip.
6. If time runs the wrong way for how the strip is mounted, tick **Reverse direction** in the web UI. By default midnight is at the end where the strip's cable is connected.
7. The lamp uses Swedish time by default. If it's somewhere else, choose **Time zone** in the web UI: Sweden/Norway/Denmark/Central Europe, Finland/Baltics, United Kingdom/Ireland, or UTC. Daylight saving time is handled automatically.

### Finding the lamp on the network

| Where | Name |
|---|---|
| Browser | `http://elpris-led.local` |
| Router's device list | `elpris-led-xxxxxx`, where `xxxxxx` is the end of the board's MAC address |
| Arduino IDE network ports (OTA) | `elpris-led-xxxxxx`, once an OTA password is set |
| The board's blue LED | The end of the IP address as counted blinks, three times after each start |

With several lamps on the same network, only one of them answers to `elpris-led.local`. Use the router names and IP addresses for the others; the web UI of each lamp shows its own name and IP address. If your router has a saved name or a fixed IP for the board from an earlier project, it may show that name instead; rename the entry in the router.

## Reading the strip

The lamp has two color modes, chosen under **Colors** in the web UI.

### Compared with a normal price (default)

The question it answers: does it matter when I run the dishwasher, the washing machine or the car charger? Designed by [@sealjo](https://github.com/sealjo); the full design is in [docs/normal-price-mode.md](docs/normal-price-mode.md).

- The strip shows **today from midnight to midnight**, except the last three LEDs.
- Each LED is compared with a **normal price**: the average of the last three days, today and tomorrow (once tomorrow's prices are published). The web page shows the normal price.
- **Yellow:** within ±20 öre of normal. It doesn't matter when you use power.
- **Yellow-green / green:** 20 / 80 öre or more cheaper than normal. A price of zero or below is always green.
- **Orange / red:** 20 / 80 öre or more expensive than normal.
- **The last three LEDs:** one off LED, then **tomorrow 08–17** and **tomorrow 17–24**, each compared with the same hours today. Green means tomorrow is cheaper, red more expensive, yellow about the same. They are off until tomorrow's prices are published, usually after lunch.

A day where prices only move between 30 and 40 öre is all yellow, because it really doesn't matter; a day with a big evening peak shows it in red.

### Cheapest to most expensive

The coloring from v3 and earlier: today, and tomorrow once published, from green (the cheapest time in the period shown) to red (the most expensive). Even a cheap day has red hours, which is good for finding the very cheapest time, for example for charging a car.

### In both modes

- The LED for **the current 15 minutes pulses towards white**.
- **Time that has passed is dimmed** (to 10 % by default), so the hours that are left stand out.

Try both modes, and all the settings, in the [emulator](https://flysen.github.io/elpris-led/emulator/).

## Settings in the web UI

| Setting | What it does |
|---|---|
| Price area | SE1–SE4, the bidding zone whose prices are shown |
| Number of LEDs | How many LEDs the strip has (up to 300); changing it restarts the lamp |
| Colors | *Compared with a normal price* (default) or *Cheapest to most expensive* |
| Normal price: past days | 0–7 past days in the normal price, default 3 |
| Doesn't matter within | The yellow zone around the normal price, default ±20 öre |
| Very cheap or expensive from | Where green and red start, default ±80 öre |
| Levels | 5 (green, yellow-green, yellow, orange, red) or 3 (green, yellow, red) |
| Count tomorrow in the normal price | On by default. Today's colors may then change when tomorrow's prices arrive |
| Reverse direction | Which end of the strip is midnight |
| Pulse the current 15 minutes towards white | Off: the current LED pulses in its own price color instead |
| Brightness of passed time | 0–100 % of normal brightness, default 10 % |
| Time zone | Where the lamp is, so the strip starts at local midnight; Swedish time by default |
| Use night mode | On by default. Off: the day brightness is used around the clock, e.g. in a laundry room |
| Night mode start / end | Hours when the night brightness is used |
| Day / night brightness | Overall brightness of the strip, 0–255 |
| Install new releases automatically | On by default |
| Show status on the board's blue LED | On by default. Off: the LED stays dark, except while setting up wifi |
| Also install test versions | Off by default. On: release candidates are installed too, wirelessly (see [Updating](#updating)) |
| OTA password | Turns on updates from the Arduino IDE; empty means off |
| Config portal password | Protects the *Elpris-LED-Config* wifi that opens when the lamp can't reach your network |

All prices are spot prices excluding VAT, grid fees and energy tax. They come from ENTSO-E in euro, converted to SEK by Elpriset just nu.se, so they can differ slightly from your electricity supplier's figures.

## Features

- Two color modes: compared with a normal price, with a yellow "doesn't matter" zone and tomorrow compared with today, or cheapest to most expensive.
- "Now" is easy to see: a white pulse on the current 15 minutes, and dimmed past hours with adjustable brightness.
- The direction can be reversed, so time reads left to right however the strip is mounted.
- Time zone chosen from a list with plain names; daylight saving time and 23- and 25-hour days are handled.
- All settings in a web UI at `http://elpris-led.local` (see the table above).
- Shows up as `elpris-led-xxxxxx` in the router, so it's easy to find.
- Updates itself: new releases are found within six hours and installed automatically. Testers can opt in to release candidates.
- The board's blue LED shows what the lamp is doing, and its network address after it starts.
- Few API calls; cached prices are used if fetching fails, with automatic retries. A day only counts as fetched when all of it arrived, and the web UI shows the reason when fetching fails.
- OTA updates and a config portal password, both set in the web UI. No passwords in the code.
- An emulator for trying the color modes and settings in a browser, on today's real prices.

## Status colors

| What you see | Meaning |
|---|---|
| Whole strip pulses blue | Waiting for time or prices |
| Whole strip pulses purple | No prices for now and fetching fails |
| First three LEDs (from midnight) blink magenta | Fetching fails, cached prices still shown |
| Dim cyan dots | Installing an update, the lamp restarts when done |

### The board's blue LED

The small blue LED on the ESP32 board (GPIO2) shows what the lamp is doing, also when the strip is dark at night. Try the patterns in the [emulator](https://flysen.github.io/elpris-led/emulator/), under *Advanced*.

| What you see | Meaning |
|---|---|
| Fast blinking | Connecting to wifi, or the lamp's own wifi *Elpris-LED-Config* is open for setup |
| Counted blinks, three times after starting | The lamp's IP address, see below |
| On | Fetching prices or installing an update |
| Double blinks | Fetching prices fails; the web page shows why |
| A short blink every 10 seconds | Everything works |

**Reading the address:** the lamp blinks the parts of its IP address that differ between devices on your network: the last part on a typical home network (255.255.255.0), the last two on a 255.255.0.0 network. Each digit is that many short blinks, **0 is one long blink**, there is a short pause between digits and a long pause between the parts. For example, 172.16.1.90 on a 255.255.0.0 network is one blink, a long pause, nine blinks, a short pause and one long blink: *1.90*. Restart the lamp to see the address again.

The LED can be turned off under *Device* in the web UI. Setting up wifi always blinks, so a lamp can be set up anyway.

## Hardware

**Board:** an **ESP32** with 4 MB flash, for example ESP32 DevKit, ESP32-WROOM-32 or NodeMCU-32. Not ESP32-C3, -S2 or -S3. The strip's data goes to **GPIO16** (marked D16 or RX2). The board's blue LED on GPIO2 shows the lamp's status; boards without it work the same.

**ESP8266** boards were supported up to v4.0.5. Lamps on ESP8266 keep running v4.0.5 but get no further updates: every six hours they see the newer release, restart to install it and find no ESP8266 file, so they start again on v4.0.5. That takes a few seconds and does no harm. To get new versions, move the lamp to an ESP32 board, which costs about the same.

Many ESP32 DevKit boards are too wide for a standard breadboard. Female Dupont wires straight onto the pins work well: only RX2, GND and, if the board powers the strip, VIN are needed.

**Also needed:**

- WS2812B strip, 5 V, up to 300 LEDs, and a 5 V supply sized for it (up to 60 mA per LED at full white)
- 330–470 Ω resistor in the data line, 1000 µF capacitor across +5 V/GND at the strip input

```
5 V supply          ESP32                      WS2812B strip (DIN end)
  +5V ─────┬──────── 5V / VIN
           └────────────────────────────────── +5V
  GND ─────┬──────── GND
           └────────────────────────────────── GND
                     GPIO16 ── 330 Ω ──────── DIN
                     (marked D16 or RX2)
```

The board and the strip must share ground. Don't power the strip from the board's 3V3 pin. Connect the strip at the end where the arrows point away from you (DIN). The strip can be mounted with that end on either side; use *Reverse direction* in the web UI to make time run left to right. Don't power the board from USB and the 5 V supply at the same time unless your board allows it: install over USB first, then connect the supply.

## Updating

- **Automatic (default):** the lamp checks for a new release two minutes after start and then every six hours. When it finds one, it downloads and installs it right away and restarts into the new version. This takes about a minute, with dim cyan dots on the strip and the blue LED on. Wifi and settings are kept. If the download fails, the lamp carries on with its current version and tries again at the next check.
- **Check for updates now:** the button in the lamp's web UI does the same thing right away. The page reloads by itself while it checks and installs, and shows the current version, board and latest release. You can also turn automatic updates off there.
- **Test channel:** tick *Also install test versions (release candidates)* in the web UI. The lamp then asks GitHub for the newest release, release candidates such as `v4.1-rc1` included, and installs it from the release's `-ota.bin` file, wirelessly. Untick it to go back; the lamp then stays on its version until the next stable release, which it installs as usual.
- **Install page:** run it again and choose *not* to erase. Wifi and settings are kept.
- **Arduino OTA:** if you've set an OTA password, the lamp shows up as `elpris-led-xxxxxx` among the network ports in the Arduino IDE. You can also upload `elpris-led-<version>-<board>-ota.bin` from [Releases](https://github.com/flysen/elpris-led/releases) with `espota.py`.

Builds you compile yourself, and CI builds, report version `dev` and never update automatically, so your own changes aren't overwritten. Flash a release to get back on automatic updates.

## Emulator

[flysen.github.io/elpris-led/emulator](https://flysen.github.io/elpris-led/emulator/) (source: `docs/emulator/index.html`) runs the lamp's color logic in the browser: both color modes, tomorrow's LEDs, dimming, the white pulse, reverse direction and night mode. It opens on **today's real prices** for the chosen price area, at the real time of day; example days and pasting prices yourself are also there. It is published together with the install page at every stable release. To see the version on `main` before that, open [raw.githack.com/flysen/elpris-led/main/docs/emulator/index.html](https://raw.githack.com/flysen/elpris-led/main/docs/emulator/index.html).

**Where the real prices come from:** a browser page can't fetch prices from elprisetjustnu.se directly, so a scheduled GitHub workflow, `.github/workflows/prices.yml`, runs `scripts/fetch-prices.sh` every hour. It fetches today, tomorrow (once published) and the last three days for SE1–SE4 and publishes them as one small file, `prices.json`, on the branch `prices`. The emulator reads it from [raw.githubusercontent.com/flysen/elpris-led/prices/prices.json](https://raw.githubusercontent.com/flysen/elpris-led/prices/prices.json). The branch always has a single commit, so it doesn't grow. If the file can't be read, the emulator uses the example days. The lamps don't use this file; they fetch their own prices.

GitHub pauses scheduled workflows after 60 days without activity in the repository and sends an e-mail about it; *Enable workflow* in the Actions tab starts it again. The workflow can also be run by hand there (*Run workflow*).

The emulator repeats the lamp's logic in JavaScript, so when what the strip shows changes, the emulator must be updated in the same pull request.

## Building from source

Only needed if you want to change the code. The sketch is in `elpris_led/`; board-specific includes and pins are in `elpris_led/platform.h`. With [arduino-cli](https://arduino.github.io/arduino-cli/) installed:

```bash
bash scripts/arduino-deps.sh                # ESP32 core and libraries at the pinned versions
bash scripts/build-firmware.sh dev esp32    # build and check the firmware into build-out/esp32 and dist/
bash scripts/package.sh dev                 # install page and emulator with manifest.json in site/, and dist/SHA256SUMS
```

To upload directly to a connected board:

```bash
arduino-cli upload --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs -p /dev/ttyUSB0 --input-dir build-out/esp32 elpris_led
```

In the Arduino IDE: add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` under *Preferences*, install *esp32* in the Boards Manager and FastLED, ArduinoJson and WiFiManager (by tzapu) in the Library Manager, then open `elpris_led/elpris_led.ino`. Choose the board *ESP32 Dev Module* and the *Partition Scheme* **Minimal SPIFFS (1.9MB APP with OTA)**.

Use the versions listed in `scripts/arduino-deps.sh` to get the same result as a release.

The logic that doesn't need the hardware is in `elpris_led/elpris_logic.h` and is tested on a regular computer: time and daylight saving time, 15-minute slots, local-hour periods, LED order and direction, past/now/future, dimming, the normal price and its color levels, tomorrow's LEDs, the time zone list (offsets and DST change days for every zone), settings carried over from older versions, night mode, version comparison, the order of releases in the test channel, and the blue LED's patterns and address blinks.

```bash
make test            # unit tests
make test-sanitize   # the same tests with AddressSanitizer and UndefinedBehaviorSanitizer
make coverage        # how much of elpris_logic.h the tests run: at least 95 % of lines, 90 % of branches
```

To add a time zone, add it to `TIME_ZONES` in `elpris_logic.h` and its expected offsets to `testTimeZones()` in `tests/test_logic.cpp`; the tests fail if a listed zone has no expected values.

### Continuous integration

Every pull request and every push to `main` runs:

- **tests:** the unit tests, normally and with sanitizers, and their coverage of `elpris_logic.h`. The numbers are shown on the run's summary page; the check fails below 95 % of lines or 90 % of branches.
- **lint:** shellcheck on the build scripts.
- **firmware:** the same build and packaging as a release, with pinned dependencies. It checks that the app uses less than 95 % of its space (warning above 85 %), that the version is in the binary, and that the install page manifest is valid. The sizes are shown on the run's summary page, and the firmware is attached to the run for 14 days so you can try a pull request on your own lamp before releasing it.

Dependabot opens pull requests when the GitHub Actions used here get new versions. The core and library versions are bumped by hand in `scripts/arduino-deps.sh`.

### Contributing

The rules for contributing, for people and AI assistants alike, are in [AGENTS.md](AGENTS.md); `CLAUDE.md` points there too. In short:

- **Every pull request has test cases.** The pull request template (`.github/pull_request_template.md`) asks for them: automated tests for new logic, and checks by hand on a lamp, in the emulator or in the browser, written so someone else can repeat them. Each test case is a checkbox that is ticked when it has passed.
- Documentation (README, install page, emulator, web UI help texts) is updated in the same pull request as the code, and `CHANGELOG.md` gets a line under *Unreleased* when something changes for people who use the lamp.
- Bigger changes to what the strip shows are best described in a design document under `docs/` first, like [docs/normal-price-mode.md](docs/normal-price-mode.md).
- Changes to the files in `.github/workflows/` need GitHub's *workflow* permission, which some tools and integrations don't have; such changes may have to be made by hand in the browser.

## Releases (for maintainers)

Pushing a version tag checks the tag, runs the tests, builds the firmware with that version number, publishes a release with `-full.bin` and `-ota.bin` plus `SHA256SUMS`, and updates the install page and the emulator. Lamps in the field pick up the new version from the install page within six hours.

```bash
git tag v2.1
git push origin v2.1
```

Or in the browser: *Releases → Draft a new release*, click *Tag*, type a new tag, click *Create new tag*, choose `main` and publish.

Before tagging, rename *Unreleased* in `CHANGELOG.md` to the new version (in a pull request), and paste that section into the release notes.

The release stops before anything is published if:

- the tag isn't `v2.1`, `v2.1.3` or `v2.1-rc1`,
- a stable tag's commit isn't on `main` (release candidates may be tagged on a pull request's branch, to try a change on a lamp before merging),
- a stable tag isn't higher than the previous stable release (lamps would ignore it),
- any test or build check fails.

**Release candidates** (`v2.1-rc1`) are published as pre-releases only. They don't update the install page, and lamps on the normal channel never install them. Lamps with *Also install test versions* ticked install them with *Check for updates now*, or within six hours; others can flash the `-full.bin` by hand. When it works, tag `v2.1` on the same commit. A lamp running `v2.1-rc1` moves on to `v2.1` by itself, on either channel.

**Flash layout:** ESP32 uses the *Minimal SPIFFS* partition scheme (two 1.9 MB app slots). Automatic updates only replace the app, never the flash layout, so it must not change in a release. If they ever have to, lamps need a USB install from the install page.

**Install page paths:** lamps download `firmware/esp32/elpris-led.bin`. Lamps on v2.0 use `firmware/elpris-led.bin`, which the build keeps as a copy of the app. Lamps on the test channel download `elpris-led-<tag>-esp32-ota.bin` from the release itself, so those file names must not change either. From v5.0 there is no `firmware/esp8266/` and no ESP8266 file in releases, so ESP8266 lamps on v4.0.x stay where they are.

One-time repository setup before the first release:

1. *Settings → Pages*: set *Source* to *GitHub Actions*.
2. *Settings → Environments → github-pages*: allow tags matching `v*` under deployment branches and tags. Otherwise the deploy is blocked because it runs from a tag.
3. *Settings → Rules → Rulesets*: protect `main` and require the CI checks *tests*, *lint* and *firmware* to pass before merging. Leave the `prices` branch unprotected, so the prices workflow can replace it every hour.

## Credits

- Elpriser tillhandahålls av [Elpriset just nu.se](https://www.elprisetjustnu.se).
- The **normal-price color mode** and the **emulator** were designed and built by [@sealjo](https://github.com/sealjo). The design is in [docs/normal-price-mode.md](docs/normal-price-mode.md), translated from his [original proposal](https://github.com/sealjo/elpris-led/blob/emulator/docs/emulator-forslag/FORSLAG.md).

## License

See [LICENSE](LICENSE).
