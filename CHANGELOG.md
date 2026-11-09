# Changelog

What's new in each release of Elpris-LED, written for people who use the lamp. Lamps install new releases automatically within six hours.

## Unreleased

- Tests for every time zone in the list (winter and summer offsets, daylight saving change days) and for keeping the dimming setting from v3.0. No change in how the lamp behaves.
- A pull request checklist and this changelog, so documentation is updated together with the code.

## v3.1

- **Brightness of passed time** can be set from 0 % (off) to 100 % (not dimmed). The default is now 10 %, which makes the hours that are left stand out more. Lamps that had dimming turned off in v3.0 keep full brightness.
- **Simpler time zone setting:** one list with plain names (Sweden/Central Europe, Finland/Baltics, UK/Ireland, UTC) that shows the current choice, instead of a technical time zone code. The time zone is no longer asked for when setting up wifi; Swedish time is the default.

## v3.0

- **"Now" is easier to see:** the LED for the current 15 minutes pulses towards white, and time that has passed is dimmed. Both can be turned off.
- **More reliable automatic updates:** a new release is installed right after a restart, when the most memory is free. This matters most on ESP8266. The lamp checks every six hours instead of once a day.
- The lamp's web page reloads by itself while it checks for and installs updates.

## v2.5

- Same firmware as v2.4.

## v2.4

- **Reverse direction:** time can run either way along the strip, so it reads left to right however the strip is mounted.

## v2.3

- Fixed fetching prices on ESP8266: prices are now read one at a time, which needs much less memory.
- When fetching prices fails, the web page shows why, and how much memory is free.

## v2.2

- The lamp is called **elpris-led-xxxxxx** in the router and can be opened at **http://elpris-led.local**.

## v2.1

- **ESP8266 support** (NodeMCU, Wemos D1 mini with 4 MB flash), alongside ESP32. The install page detects the board and installs the right firmware.

## v2.0

First stable release.

- Prebuilt, tested firmware for ESP32 and an install page in the browser, so nothing needs to be compiled.
- **Automatic updates** from GitHub releases.
- Today's and tomorrow's 15-minute prices along the strip, from green (cheap) to red (expensive), with daylight saving time handled.
- Web UI for price area, number of LEDs, night mode, brightness and passwords.
