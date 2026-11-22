# Instructions for AI coding assistants

This file tells AI assistants (Claude Code, GitHub Copilot, Codex, Cursor and others) how this project works. People are welcome to read it too; the human-facing documentation is [README.md](README.md).

## What this is

Elpris-LED: firmware for an ESP32 driving a WS2812B LED strip that shows the day's Swedish spot electricity prices from [elprisetjustnu.se](https://www.elprisetjustnu.se) as colors. Users install prebuilt firmware from the install page and the lamps update themselves from GitHub releases. The lamps run unattended in people's homes for years, so reliability and backwards compatibility matter more than new features.

## Where things are

| Path | What it is |
|---|---|
| `elpris_led/elpris_led.ino` | The sketch: wifi, fetching prices, updates, web UI, drawing the strip |
| `elpris_led/elpris_logic.h` | Pure logic with no Arduino dependencies: time and DST, slots, LED mapping, color levels, normal price, versions, the blue status LED's patterns. Tested |
| `elpris_led/elpris_text.h` | The web page's texts in English and Swedish, and how the page's language is chosen. Tested |
| `elpris_led/platform.h` | Board-specific includes, pins and small helpers (ESP32) |
| `tests/test_logic.cpp`, `Makefile` | Unit tests for `elpris_logic.h`, run on a normal computer |
| `scripts/` | Build, packaging, release-tag check, dependency pins (`arduino-deps.sh`) and the emulator's price fetcher |
| `.github/workflows/` | `compile.yml` (CI), `release.yml` (tag → release + install page), `prices.yml` (hourly prices for the emulator) |
| `.github/pull_request_template.md` | Summary, changes and test cases for every pull request |
| `docs/index.html` | The install page (GitHub Pages) |
| `docs/emulator/index.html` | The emulator: the lamp's color logic in JavaScript, on real prices |
| `docs/normal-price-mode.md` | Design of the default color mode |
| `CHANGELOG.md`, `README.md` | User-facing history and documentation |

## Rules that must not be broken

1. **English only** in the repository: code, comments, docs, commit messages and pull requests. (The maintainers may talk to you in Swedish; answer them in their language, but write English into the repo.) The one exception is **translations of UI text**: the lamp's web page is in English and Swedish, and every text is a line in `elpris_led/elpris_text.h` with both languages. Write the English first, then the Swedish; never put UI text straight into the sketch. The config portal stays in English.
2. **No passwords or keys in the code.** OTA and config portal passwords are set in the lamp's web UI and stored on the device.
3. **Lamps in the field must keep working after an update:**
   - Never change the **flash layout**: ESP32 *Minimal SPIFFS* (`min_spiffs`). Over-the-air updates can't change it.
   - Never rename the **update files**: `firmware/<board>/elpris-led.bin` on the install page, `firmware/elpris-led.bin` (kept for v2.0 lamps), and `elpris-led-<tag>-<board>-ota.bin` / `-full.bin` in releases.
   - **Settings from older versions must still load.** Settings keys (namespace `elpris-config`, keys at most 15 characters) are never reused for something else; when a setting changes shape, migrate it (see `initialPastPercent()`), with tests.
   - Don't change the update URLs or how versions are compared without a plan for every lamp already out there.
4. **Every pull request has test cases** (see below). New logic goes in `elpris_logic.h` with automated tests in `tests/test_logic.cpp`: boundaries, daylight saving days (23/25 hours, 92/100 quarters) and edge cases. Keep hardware code thin.
5. **The emulator mirrors the lamp.** When what the strip shows changes, update `docs/emulator/index.html` in the same pull request.
6. **ESP32 only.** ESP8266 support ended with v4.0.5; don't add it back. ESP8266 lamps on v4.0.x find newer releases, restart to install them and find no ESP8266 file, so they keep running v4.0.x (a short restart every six hours). The board's blue LED (GPIO2) and the emulator's preview of it use the same patterns: change both together.
7. **Prices are spot prices excluding VAT, grid fees and tax**, from elprisetjustnu.se. Credit them as *"Elpriser tillhandahålls av Elpriset just nu.se"* with a link where prices are shown publicly.

## Test cases

No pull request without test cases. In the pull request's *Test cases* section, list every test case as a checkbox and tick it when it has passed; a pull request is ready when all are ticked.

- **Automated** wherever there is logic: name the tests added or changed in `tests/test_logic.cpp`, and always "CI passes: tests, lint, firmware".
- **Coverage:** the tests must run at least **95 % of the lines** and **90 % of the branches** in `elpris_logic.h`. CI measures it on every pull request (`make coverage`, numbers on the run's summary page) and fails below the limits. Add tests with new logic so the numbers don't drop; don't lower the limits to make a pull request pass.
- **By hand** for what can't be tested automatically, written so someone else can repeat it: what to do and what should happen. For example "Emulator: choose 3 levels → the legend shows green, yellow, red", or "Lamp: Check for updates now → restarts and shows the new version".
- **Fetching prices and installing updates** are what keep lamps working in people's homes. Changes there get unit tests for their logic (like `parsePrices()` with the answers in `tests/fixtures/`) and go out as a **release candidate** on a lamp (installed with `espota.py` or through the test channel) for at least a day before they are merged: tag `vX.Y.Z-rcN` on the pull request's branch (release candidates may be tagged on a branch, stable releases only on `main`). No admin bypass for these.
- Changes to documentation only still get a test case, such as "Links in the README open the right pages".

## How to work

- **Branch and pull request** against `main`; `main` is protected and needs the CI checks `tests`, `lint` and `firmware`. Pull requests are squash-merged.
- **Update the documentation in the same pull request** as the code: `README.md`, the install page, the emulator, the help texts in the lamp's web UI, as far as the change affects them.
- Add a line to **`CHANGELOG.md` under *Unreleased*** when something changes for people who use the lamp, written for them, not for developers.
- Keep **commit messages short** and descriptive.
- **Bigger changes to what the strip shows** start as a design document in `docs/` (like `docs/normal-price-mode.md`), agreed before the code.
- **Credit contributors** with `Co-authored-by:` lines when you build on someone else's work, and in the README's *Credits*.
- Don't add dependencies lightly. Core and library versions are pinned in `scripts/arduino-deps.sh`.
- **Files in `.github/workflows/`** need GitHub's *workflow* permission. If your tools don't have it, write the file content in the pull request or chat and let a maintainer add it by hand.

## Commands

```bash
make test                                    # unit tests
make test-sanitize                           # the same with AddressSanitizer and UBSan
make coverage                                # how much of elpris_logic.h the tests run
bash scripts/arduino-deps.sh                 # install the pinned ESP32 core and libraries
bash scripts/build-firmware.sh dev esp32     # build one board into build-out/<board> and dist/
bash scripts/package.sh dev                  # install page, emulator and manifest into site/
```

CI runs the same on every pull request. Local and CI builds report version `dev` and never auto-update.

## Things learned the hard way

- **Arduino prototypes:** the Arduino builder declares the sketch's functions near the top of the file, before types defined in the sketch. A struct used in a function signature must be defined in a header (see `HistoryDay` in `elpris_logic.h`).
- **`DATA_PIN` is a `constexpr`, not a macro;** FastLED uses that name internally.
- **Default arguments** on sketch functions can break the generated prototypes; pass all arguments explicitly.
- **Timer callbacks:** the blue LED is driven by a `Ticker`, which runs in another task. It only reads flags (`volatile`) and writes the LED; keep it that way, no network, strip or `String` work in it.
- **HTTP answers are read with `startGet()` and `readBody()`** in the sketch: HTTP/1.0 with `Connection: close`, the whole answer read as fast as it arrives, and a hard 15-second limit. Don't parse while reading (the price server closes the connection right after a cached answer, which cut the lamp off after a few prices in v5.0-v5.1), and don't use `HTTPClient::getString()` (over HTTP/1.1 the price server keeps the connection open and chunks the answer, and v5.1.1 could hang for minutes). Nothing in the loop may block for long: the web page, OTA and the strip stop while it does.
- **Firmware is downloaded with `downloadFirmware()`, not `HTTPUpdate`.** `HTTPUpdate` checks the first byte before it has arrived and reports "Verify Bin Header Failed" for a good file.
- **Release candidates** (`vX.Y-rcN`) are pre-releases: the install page and normal lamps ignore them; lamps with the test channel install them.

## Releases

Release candidates can be tagged on a pull request's branch (choose the branch as target); stable releases only on `main`. Maintainers release from the browser: rename *Unreleased* in `CHANGELOG.md` to the version in a pull request and merge it (`check-release-tag.sh` stops a stable tag whose commit has no heading for it in `CHANGELOG.md`), then *Releases → Draft a new release*, tag `vX.Y[.Z]` (or `vX.Y-rcN` as a pre-release) on `main`, and paste the changelog section as release notes. `release.yml` checks the tag, runs the tests, builds the firmware and publishes the release and the install page.
