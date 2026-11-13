# Design: compared with a normal price

The color mode *Compared with a normal price*, with two LEDs for tomorrow, was proposed and designed by [@sealjo](https://github.com/sealjo). This is his proposal, translated from Swedish, with notes on how the firmware implements it. The original is [FORSLAG.md in sealjo/elpris-led](https://github.com/sealjo/elpris-led/blob/emulator/docs/emulator-forslag/FORSLAG.md), branch `emulator`.

The proposal is meant for someone who wants to see at a glance whether it pays to move the laundry, the dishes or a bath. It can be tried in the [emulator](emulator/index.html).

## The idea

The other color mode colors each day from the cheapest (green) to the most expensive (red) within what is shown. A day where the price only moves between 30 and 40 öre then becomes just as red and green as a day between 20 and 300 öre, although in practice it doesn't matter when you do the laundry. This mode compares with a normal price instead, and has a neutral zone for small differences. It also answers the question "is tomorrow cheaper or more expensive than today?" with two LEDs of its own.

## Layout on the strip

With `N` LEDs:

| LEDs | Shows |
|---|---|
| `0` to `N-4` | Today from local midnight to midnight, as in the other mode but on `N-3` LEDs |
| `N-3` | Always off, separates today from tomorrow |
| `N-2` | Tomorrow daytime, 08–17, compared with today 08–17 |
| `N-1` | Tomorrow evening, 17–24, compared with today 17–24 |

Today's part uses the same `ledSlotRange` as the other mode, with `numLeds = N-3` and today's number of quarters (96, or 92/100 when daylight saving time starts or ends). Each LED gets the average price of its quarters. Tomorrow is not drawn on the strip; it is only summarized in the last two LEDs.

What carries over from the other mode applies to the whole strip: the current LED pulses towards white, passed time is dimmed, the night brightness is used, and reverse direction (`physicalLed`) flips the whole strip, including the last three LEDs. The two tomorrow LEDs always count as future, so they are never dimmed and never pulse.

*Implementation:* `dayLedCount()` in `elpris_logic.h`. A strip with three LEDs or fewer is too short for the tomorrow LEDs and shows only today.

## Normal price

The normal price is the average of all quarter prices for:

- the `H` most recent whole days (default 3, adjustable 0–7),
- today,
- tomorrow, once tomorrow's prices are known.

Counting tomorrow means that today's colors can change when tomorrow's prices are fetched after lunch. A very expensive tomorrow raises the normal price, and today becomes greener, which matches the advice "do the laundry today". If you want today's colors to stay put during the day, tomorrow can be left out of the normal price.

The history is fetched from the same address as today, with earlier dates, one request per day. Only a sum and a count per day are needed, not every quarter. ESP8266 only has room for 256 quarters in its cache, about 2.5 days, so the history should be kept as daily sums and not in the ring buffer.

*Implementation:* `PriceSum` and `HistoryDay` in `elpris_logic.h`; `missingHistoryDay()`, `storeHistory()` and `rebuildNormal()` in the sketch. Past days are fetched after today and before tomorrow. A past day that can't be fetched is left out of the normal price, so it never blocks tomorrow's prices. Setting: *Count tomorrow in the normal price*, on by default as proposed.

## Levels and colors

For today's LEDs: `d = price - normal price`. For the tomorrow LEDs: `d = average tomorrow - average today` for the same period. The same limits apply to both:

| Level | Condition (default) | Hue (FastLED, `CHSV(hue, 255, 255)`) | Color |
|---|---|---|---|
| Very cheap / much cheaper | `d <= -80 öre`, or price `<= 0` (today's LEDs only) | 96 | green |
| Cheap / cheaper | `d <= -20 öre` | 80 | yellow-green |
| Doesn't matter / about the same as today | `-20 < d < 20 öre` | 64 | yellow |
| Expensive / more expensive | `d >= 20 öre` | 32 | orange |
| Very expensive / much more expensive | `d >= 80 öre` | 0 | red |

The limits of 20 and 80 öre apply to the spot price excluding VAT, like everything else in the lamp. There is also a mode with only three levels (green 96, yellow 64, red 0), where "very" is merged into "cheap" and "expensive". Yellow-green and green can be hard to tell apart on a real strip, so three levels may be clearer.

Nothing blinks or breathes, except the current-time marker as before.

*Implementation:* `levelForDiff()`, `levelForPrice()` and `levelHue()` in `elpris_logic.h`, with tests for every limit. (The magenta error blink of the first LEDs when prices can't be fetched is unchanged.)

## The tomorrow LEDs

- **Periods:** 08–17 (quarter 32–67) and 17–24 (quarter 68–95) in local time. On the days daylight saving time starts or ends, the periods should be counted from local time, not from quarter numbers.
- **Before tomorrow's prices are available** (the same check as `dayCovered` for tomorrow), both are off.
- **Averages:** both today's and tomorrow's average are for the whole period, including the part of today that has already passed.

*Implementation:* `TOMORROW_PERIODS` and `localHourSlot()` in `elpris_logic.h`; periods are counted from local hours, with tests for the daylight saving time days.

## Settings in the web UI

| Setting | Default |
|---|---|
| Normal price, past days | 3 |
| Doesn't matter within | ±20 öre |
| Very expensive or cheap from | ±80 öre |
| Number of levels | 5 (or 3) |
| Count tomorrow in the normal price | on |

All five are in the lamp's web UI under *Colors*.

## Simplifications in the emulator

The emulator counts 96 quarters per day and ignores days with 23 or 25 hours. The example days and the seven past days are made up. Pasted real prices have no history, and then the normal price is based on today and tomorrow only. The clock starts at 14:00 so the tomorrow LEDs show directly. The lamp itself starts looking for tomorrow's prices at 13:30.
