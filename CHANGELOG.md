# Changelog

What's new in each release of Elpris-LED, written for people who use the lamp. Lamps install new releases automatically within six hours.

## Unreleased

## v5.2

- **In Swedish:** the lamp's web page, the install page and the emulator. A browser set to Swedish gets Swedish, others English. On the lamp, choose the language yourself under *Device → Language / Språk*; on the install page and in the emulator, with the *Svenska / English* button. The config portal for setting up wifi stays in English, and so do the install dialogs, which come from ESP Web Tools.

## v5.1.4

- **Fixed: v5.1.1 to v5.1.3 could hang while fetching prices**, so the web page and wireless updates didn't answer and the strip stood still. The lamp now reads every answer itself and gives up after 15 seconds at most, so a slow or odd server can never hold it up.
- **Fixed: updates could fail with "Verify Bin Header Failed"** although the file was fine. The lamp now downloads the new firmware itself, checks it and tries up to three times.

## v5.1.2 and v5.1.3

- Same as v5.1.1; they were released before the fixes in v5.1.4 were merged.

## v5.1.1

- **Fixed: the lamp could stop getting prices** and pulse purple. When the price server answered from its cache, it closed the connection so fast that the lamp only read the first few prices of the day. The lamp now reads the whole answer before using it, and an answer that was cut off is never taken for a whole day. The same fix applies to the check for updates.

## v5.1

- **Night mode can be turned off:** untick *Use night mode* in the web UI and the lamp uses the day brightness around the clock. Handy where nobody sleeps, like a laundry room. Night mode is on by default, so nothing changes for lamps that are already set up. Also in the emulator.

## v5.0

- **ESP32 only.** ESP8266 boards are no longer supported. An ESP8266 lamp keeps running v4.0.5 but gets no more updates; every six hours it restarts briefly when it finds a newer release it can't install. Move the lamp to an ESP32 board to get new versions. The install page only installs on ESP32.
- **The board's blue LED shows the lamp's status:** fast blinking while setting up wifi, the end of the lamp's IP address as counted blinks three times after it starts, on while fetching prices or updating, double blinks when fetching fails, and a short blink every 10 seconds when everything works. The same patterns as in the emulator. It can be turned off in the web UI.
- **Updates install without an extra restart:** the lamp downloads and installs a new release right away and restarts once into it.
- **Tomorrow's prices are fetched from 13:00** instead of 13:30, so they show up sooner after they are published.
- The lamp keeps about ten days of 15-minute prices instead of five.
- The lamp's web page, the install page and the README credit the prices: *Elpriser tillhandahålls av Elpriset just nu.se*.
- Cleaned-up README: ESP32 only, a section on the blue LED and how to read the address.

## v4.0.5

- **Emulator:** a preview of the blue status LED planned for the ESP32 board (under *Advanced*): fast blinking while setting up wifi, the lamp's network address as counted blinks, and signals for working normally, fetching prices, fetch errors and updates. Try the patterns there before they go into the lamp. No change in the lamps' firmware.

## v4.0.4

- **The emulator is easier to read:** a summary in plain words at the top (the price now, whether it's cheaper or more expensive than usual, the cheapest time left, and how tomorrow compares with today), the lamp's settings with the same names as on the lamp's own page, and a chart with a title and units.
- The emulator shows the color mode chosen under *Colors*, like the lamp. Comparing both modes, the experiments with other color scales, your own prices and the technical details are under *Advanced*.
- Tap an LED or the chart on a phone to see its details.
- No change in the lamps' firmware.

## v4.0.3

- **The emulator shows real prices:** today's, tomorrow's (once published) and the last three days' prices for SE1–SE4, updated every hour, at the real time of day. The example days and pasting prices yourself are still there. Prices provided by [Elpriset just nu.se](https://www.elprisetjustnu.se).
- No change in the lamps' firmware.

## v4.0.2

- **ESP8266:** fetching prices and checking for updates failed on v4.0, because a secure connection needs almost all of the ESP8266's free memory. The lamp now asks each server whether it accepts smaller data packets, and then uses 4 kB instead of 16 kB of memory for the connection.
- A day's prices only count as fetched when the whole day arrived. Before, a download that stopped half-way could leave the lamp without a price for the current time while the web page said *OK*. Now it tries again and shows how much arrived.
- **ESP8266 is in maintenance mode from this version:** it keeps getting fixes like this one, but new features are developed for ESP32. An ESP32 board costs about the same and has more than ten times as much free memory. ESP32 is recommended for new lamps.

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
