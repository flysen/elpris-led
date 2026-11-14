# Changelog

What's new in each release of Elpris-LED, written for people who use the lamp. Lamps install new releases automatically within six hours.

## Unreleased

## v4.0.1

- The lamp now introduces itself as **Elpris-LED** when it fetches prices and checks for updates, and asks for JSON, instead of using the network library's anonymous default. Some sites block anonymous requests.
- When the price server answers with something that isn't prices, the web page now shows what it sent: the content type and the first characters of the response.

## v4.0

- **New default color mode: Compared with a normal price.** Today's prices are compared with the average of the last three days, today and tomorrow. Within ±20 öre of that the LED is yellow, because then it doesn't matter when you run the dishwasher; cheaper is green and more expensive is orange or red, with separate colors for "very" cheap and expensive. Idea and design by [@sealjo](https://github.com/sealjo).
- **The last three LEDs show tomorrow** in the new mode: one off LED, then tomorrow 08–17 and 17–24 compared with the same hours today. Green is cheaper than today, red more expensive. They are off until tomorrow's prices are published.
- The previous coloring is still available as **Cheapest to most expensive** in the web UI. Lamps switch to the new mode when they update; switch back in the web UI if you prefer the old one.
- New settings for the normal price: how many past days count (0–7), the yellow zone, where "very" starts, 5 or 3 color levels, and whether tomorrow counts.
- The web page shows the normal price and how tomorrow compares with today.
- **Emulator** at [flysen.github.io/elpris-led/emulator](https://flysen.github.io/elpris-led/emulator/): try both color modes and all display settings in the browser, without a lamp. By [@sealjo](https://github.com/sealjo).
- **Test channel for updates:** tick *Also install test versions* in the web UI, and the lamp installs release candidates too, wirelessly with *Check for updates now*. For testing new features before everyone else; off by default.

## v3.2

- Tests for every time zone in the list and for keeping the dimming setting from v3.0. No change in how the lamp behaves.
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
