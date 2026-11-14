// Tests for elpris_led/elpris_logic.h that run on a regular computer: make test
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../elpris_led/elpris_logic.h"

static int passed = 0, failed = 0;
static const char* SWEDEN_TZ = "CET-1CEST,M3.5.0,M10.5.0/3";   // the lamp's default

static void check(const char* name, long long actual, long long expected) {
  if (actual == expected) { passed++; printf("OK   %s\n", name); }
  else { failed++; printf("FAIL %s: got %lld, expected %lld\n", name, actual, expected); }
}

static void checkTrue(const char* name, bool cond) { check(name, cond ? 1 : 0, 1); }

static void checkNear(const char* name, double actual, double expected) {
  if (fabs(actual - expected) < 1e-6) { passed++; printf("OK   %s\n", name); }
  else { failed++; printf("FAIL %s: got %f, expected %f\n", name, actual, expected); }
}

static time_t utc(int Y, int M, int D, int h, int m) {
  struct tm t = {};
  t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D; t.tm_hour = h; t.tm_min = m;
  return timegm(&t);
}

static void useTimeZone(const char* tz) {
  setenv("TZ", tz, 1);
  tzset();
}

static void testDaysFromCivil() {
  check("daysFromCivil: 1970-01-01", daysFromCivil(1970, 1, 1), 0);
  check("daysFromCivil: 2000-03-01, after leap day", daysFromCivil(2000, 3, 1), 11017);
  check("daysFromCivil: 2026-11-05", daysFromCivil(2026, 11, 5), utc(2026, 11, 5, 0, 0) / 86400);
}

static void testParseIso() {
  time_t t = 0;
  checkTrue("parseIso: winter time +01:00 parses", parseIso("2026-11-05T00:00:00+01:00", t));
  check("parseIso: winter time +01:00 gives correct UTC", t, utc(2026, 11, 4, 23, 0));
  parseIso("2026-07-01T12:00:00+02:00", t);
  check("parseIso: summer time +02:00 gives correct UTC", t, utc(2026, 7, 1, 10, 0));
  parseIso("2026-01-01T00:15:00Z", t);
  check("parseIso: Z is treated as UTC", t, utc(2026, 1, 1, 0, 15));
  parseIso("2026-01-01T06:00:00", t);
  check("parseIso: no offset is treated as UTC", t, utc(2026, 1, 1, 6, 0));
  checkTrue("parseIso: garbage returns false", !parseIso("not a date", t));
}

static void testPriceSlots() {
  time_t a = utc(2026, 11, 5, 10, 0);
  int32_t s0, s1;
  priceSlots(a, a + 900, s0, s1);
  check("priceSlots: 15 minutes gives 1 slot", s1 - s0, 1);
  check("priceSlots: correct start slot", s0, a / SLOT_SEC);
  priceSlots(a, a + 3600, s0, s1);
  check("priceSlots: 1 hour gives 4 slots", s1 - s0, 4);
  priceSlots(a, a, s0, s1);
  check("priceSlots: missing end is assumed to be 1 hour", s1 - s0, 4);
}

static void testDayLengths() {
  time_t normal = utc(2026, 11, 5, 11, 0);     // 12:00 Swedish time
  check("localMidnightSlot: midnight 2026-11-05 Swedish time",
        localMidnightSlot(normal, 0), utc(2026, 11, 4, 23, 0) / SLOT_SEC);
  check("day length: normal day 96 slots",
        localMidnightSlot(normal, 1) - localMidnightSlot(normal, 0), 96);

  time_t spring = utc(2026, 3, 29, 10, 0);
  check("day length: 2026-03-29, DST starts, 92 slots",
        localMidnightSlot(spring, 1) - localMidnightSlot(spring, 0), 92);

  time_t autumn = utc(2026, 10, 25, 10, 0);
  check("day length: 2026-10-25, DST ends, 100 slots",
        localMidnightSlot(autumn, 1) - localMidnightSlot(autumn, 0), 100);

  time_t dayBefore = utc(2026, 3, 28, 11, 0);
  check("48 h window across the DST change: 188 slots",
        localMidnightSlot(dayBefore, 2) - localMidnightSlot(dayBefore, 0), 188);
}

// Local hours used for tomorrow's periods, in Swedish time, including the DST change days
static void testLocalHourSlot() {
  time_t winter = utc(2026, 11, 13, 12, 0);
  check("localHourSlot: 08:00 in winter is 07:00 UTC", localHourSlot(winter, 0, 8), utc(2026, 11, 13, 7, 0) / SLOT_SEC);
  check("localHourSlot: tomorrow 17:00 in winter is 16:00 UTC", localHourSlot(winter, 1, 17), utc(2026, 11, 14, 16, 0) / SLOT_SEC);
  check("localHourSlot: hour 24 is the next midnight", localHourSlot(winter, 0, 24), localMidnightSlot(winter, 1));
  check("localHourSlot: hour 0 is the same as localMidnightSlot", localHourSlot(winter, 1, 0), localMidnightSlot(winter, 1));

  time_t summer = utc(2026, 7, 15, 12, 0);
  check("localHourSlot: 08:00 in summer is 06:00 UTC", localHourSlot(summer, 0, 8), utc(2026, 7, 15, 6, 0) / SLOT_SEC);

  time_t spring = utc(2026, 3, 29, 12, 0);   // clocks go forward 02:00 -> 03:00
  check("2026-03-29: 00-08 is 7 hours (28 slots)", localHourSlot(spring, 0, 8) - localHourSlot(spring, 0, 0), 28);
  check("2026-03-29: 08-17 is 9 hours (36 slots)", localHourSlot(spring, 0, 17) - localHourSlot(spring, 0, 8), 36);
  check("2026-03-29: 17-24 is 7 hours (28 slots)", localHourSlot(spring, 0, 24) - localHourSlot(spring, 0, 17), 28);

  time_t autumn = utc(2026, 10, 25, 12, 0);  // clocks go back 03:00 -> 02:00
  check("2026-10-25: 00-08 is 9 hours (36 slots)", localHourSlot(autumn, 0, 8) - localHourSlot(autumn, 0, 0), 36);
  check("2026-10-25: 08-17 is 9 hours (36 slots)", localHourSlot(autumn, 0, 17) - localHourSlot(autumn, 0, 8), 36);

  // The day before a DST change: tomorrow's periods are counted in tomorrow's local time
  time_t dayBefore = utc(2026, 3, 28, 12, 0);
  check("tomorrow 08-17 across the DST change is 36 slots", localHourSlot(dayBefore, 1, 17) - localHourSlot(dayBefore, 1, 8), 36);
  check("tomorrow 08:00 across the DST change is 06:00 UTC", localHourSlot(dayBefore, 1, 8), utc(2026, 3, 29, 6, 0) / SLOT_SEC);
}

// Every time zone in the web UI's list: local midnight at the right UTC time in winter and summer,
// and 23/25-hour days on the EU change days (last Sunday of March and October) where DST applies.
static void testTimeZones() {
  struct Expected { const char* tz; int winterOffsetMin; int summerOffsetMin; bool dst; };
  const Expected expected[] = {
    {"CET-1CEST,M3.5.0,M10.5.0/3",   60, 120, true},
    {"EET-2EEST,M3.5.0/3,M10.5.0/4", 120, 180, true},
    {"GMT0BST,M3.5.0/1,M10.5.0",     0,  60,  true},
    {"UTC0",                         0,  0,   false},
  };
  const int expectedCount = sizeof(expected) / sizeof(expected[0]);

  check("TIME_ZONES: every listed time zone has expected values in the test", TIME_ZONE_COUNT, expectedCount);
  checkTrue("TIME_ZONES: the first (default) is Swedish time", strcmp(TIME_ZONES[0].tz, SWEDEN_TZ) == 0);

  for (int z = 0; z < TIME_ZONE_COUNT; z++) {
    const TimeZoneChoice& zone = TIME_ZONES[z];
    char name[160];

    const Expected* e = nullptr;
    for (int k = 0; k < expectedCount; k++) if (strcmp(expected[k].tz, zone.tz) == 0) e = &expected[k];
    snprintf(name, sizeof(name), "%s: has expected values in the test", zone.label);
    checkTrue(name, e != nullptr);
    if (!e) continue;

    snprintf(name, sizeof(name), "%s: has a label", zone.tz);
    checkTrue(name, zone.label[0] != '\0');
    for (int other = 0; other < z; other++) {
      snprintf(name, sizeof(name), "%s: listed only once", zone.label);
      checkTrue(name, strcmp(TIME_ZONES[other].tz, zone.tz) != 0);
    }

    useTimeZone(zone.tz);

    time_t winterNoon = utc(2026, 1, 15, 12, 0);
    snprintf(name, sizeof(name), "%s: midnight 2026-01-15 at the right UTC time", zone.label);
    check(name, localMidnightSlot(winterNoon, 0), (utc(2026, 1, 15, 0, 0) - e->winterOffsetMin * 60) / SLOT_SEC);

    time_t summerNoon = utc(2026, 7, 15, 12, 0);
    snprintf(name, sizeof(name), "%s: midnight 2026-07-15 at the right UTC time", zone.label);
    check(name, localMidnightSlot(summerNoon, 0), (utc(2026, 7, 15, 0, 0) - e->summerOffsetMin * 60) / SLOT_SEC);

    time_t spring = utc(2026, 3, 29, 12, 0);
    snprintf(name, sizeof(name), "%s: 2026-03-29 has %d slots", zone.label, e->dst ? 92 : 96);
    check(name, localMidnightSlot(spring, 1) - localMidnightSlot(spring, 0), e->dst ? 92 : 96);

    time_t autumn = utc(2026, 10, 25, 12, 0);
    snprintf(name, sizeof(name), "%s: 2026-10-25 has %d slots", zone.label, e->dst ? 100 : 96);
    check(name, localMidnightSlot(autumn, 1) - localMidnightSlot(autumn, 0), e->dst ? 100 : 96);

    time_t ordinary = utc(2026, 11, 9, 12, 0);
    snprintf(name, sizeof(name), "%s: an ordinary day has 96 slots", zone.label);
    check(name, localMidnightSlot(ordinary, 1) - localMidnightSlot(ordinary, 0), 96);
  }

  useTimeZone(SWEDEN_TZ);
}

static void testLedSlotRange() {
  const int windows[] = {92, 96, 100, 192};
  const int counts[]  = {1, 7, 60, 100, 300};
  for (int w : windows) {
    for (int n : counts) {
      bool ok = true;
      int end = 0;
      for (int i = 0; i < n; i++) {
        int s0, s1;
        ledSlotRange(i, w, n, s0, s1);
        if (s1 <= s0 || s0 < 0 || s1 > w) ok = false;
        if (s0 > end) ok = false;            // gap
        if (s1 > end) end = s1;
      }
      if (end != w) ok = false;
      char name[96];
      snprintf(name, sizeof(name), "ledSlotRange: %d LEDs over %d slots without gaps", n, w);
      checkTrue(name, ok);
    }
  }
}

static void testLedTimeState() {
  check("ledTimeState: LED before now is past", ledTimeState(0, 4, 10), LED_PAST);
  check("ledTimeState: LED ending exactly at now is past", ledTimeState(6, 10, 10), LED_PAST);
  check("ledTimeState: now at the LED's first slot", ledTimeState(10, 14, 10), LED_NOW);
  check("ledTimeState: now at the LED's last slot", ledTimeState(10, 14, 13), LED_NOW);
  check("ledTimeState: LED after now is future", ledTimeState(14, 18, 10), LED_FUTURE);

  // Exactly one LED is "now" for every slot, and past/future are on the right sides of it
  const int windows[] = {96, 192};
  const int counts[]  = {7, 60, 100, 300};
  for (int w : windows) {
    for (int n : counts) {
      bool ok = true;
      for (int cur = 0; cur < w; cur++) {
        int nowCount = 0, nowIdx = -1;
        for (int i = 0; i < n; i++) {
          int s0, s1;
          ledSlotRange(i, w, n, s0, s1);
          LedTime st = ledTimeState(s0, s1, cur);
          if (st == LED_NOW) { nowCount++; nowIdx = i; }
          if (st == LED_PAST && nowIdx >= 0) ok = false;     // past after now
          if (st == LED_FUTURE && nowIdx < 0) ok = false;    // future before now
        }
        // With more LEDs than slots, several LEDs share the current slot; with fewer, exactly one
        if (n <= w ? nowCount != 1 : nowCount < 1) ok = false;
      }
      char name[96];
      snprintf(name, sizeof(name), "ledTimeState: %d LEDs over %d slots, now marked correctly", n, w);
      checkTrue(name, ok);
    }
  }
}

static void testPhysicalLed() {
  check("physicalLed: normal, first LED", physicalLed(0, 100, false), 0);
  check("physicalLed: normal, last LED", physicalLed(99, 100, false), 99);
  check("physicalLed: reversed, first in time is last on strip", physicalLed(0, 100, true), 99);
  check("physicalLed: reversed, last in time is first on strip", physicalLed(99, 100, true), 0);
  check("physicalLed: reversed, middle", physicalLed(40, 100, true), 59);
  check("physicalLed: reversed, single LED", physicalLed(0, 1, true), 0);

  // Every LED is used exactly once in both directions
  const int counts[] = {1, 7, 100, 300};
  for (int n : counts) {
    for (int r = 0; r < 2; r++) {
      static bool seen[300];
      for (int i = 0; i < n; i++) seen[i] = false;
      bool ok = true;
      for (int i = 0; i < n; i++) {
        int p = physicalLed(i, n, r == 1);
        if (p < 0 || p >= n || seen[p]) ok = false; else seen[p] = true;
      }
      char name[96];
      snprintf(name, sizeof(name), "physicalLed: %d LEDs %s, each LED used once", n, r ? "reversed" : "normal");
      checkTrue(name, ok);
    }
  }
}

static void testPercentToScale() {
  check("percentToScale: 0 % is off", percentToScale(0), 0);
  check("percentToScale: 10 % (default for passed time)", percentToScale(10), 26);
  check("percentToScale: 25 % (v3.0 dimming)", percentToScale(25), 64);
  check("percentToScale: 50 %", percentToScale(50), 128);
  check("percentToScale: 100 % is full", percentToScale(100), 255);
  check("percentToScale: below 0 is clamped to off", percentToScale(-5), 0);
  check("percentToScale: above 100 is clamped to full", percentToScale(150), 255);

  // Higher percentage never gives a lower scale
  bool ok = true;
  for (int p = 1; p <= 100; p++) {
    if (percentToScale(p) < percentToScale(p - 1)) ok = false;
  }
  checkTrue("percentToScale: never decreases from 0 to 100 %", ok);
}

static void testInitialPastPercent() {
  check("initialPastPercent: new lamp gets the default", initialPastPercent(false, 0, true, 10), 10);
  check("initialPastPercent: v3.0 lamp with dimming on gets the default", initialPastPercent(false, 0, true, 10), 10);
  check("initialPastPercent: v3.0 lamp with dimming off keeps full brightness", initialPastPercent(false, 0, false, 10), 100);
  check("initialPastPercent: a saved value wins", initialPastPercent(true, 35, true, 10), 35);
  check("initialPastPercent: a saved 0 % (off) wins", initialPastPercent(true, 0, true, 10), 0);
  check("initialPastPercent: a saved value wins over old dimming off", initialPastPercent(true, 50, false, 10), 50);
}

// ---------- Normal-price color mode ----------

static void testPriceSum() {
  PriceSum empty;
  checkTrue("PriceSum: a new sum is empty", empty.empty());
  checkNear("PriceSum: an empty sum has mean 0", empty.mean(), 0.0);

  PriceSum s;
  s.add(1.0f, 4);          // an hourly price covers four quarters
  s.add(3.0f);
  check("PriceSum: counts quarters", s.count, 5);
  checkNear("PriceSum: mean is weighted per quarter", s.mean(), 7.0 / 5.0);

  PriceSum day1, day2, all;
  for (int i = 0; i < 96; i++) day1.add(0.50f);
  for (int i = 0; i < 92; i++) day2.add(1.50f);   // a 23-hour day has fewer quarters
  all.add(day1);
  all.add(day2);
  check("PriceSum: adding sums adds their quarters", all.count, 188);
  checkNear("PriceSum: mean over days weighs each quarter equally", all.mean(), (96 * 0.5 + 92 * 1.5) / 188.0);

  PriceSum negative;
  negative.add(-0.10f, 2);
  negative.add(0.30f, 2);
  checkNear("PriceSum: negative prices count in the mean", negative.mean(), 0.10);
}

static void testLevelForDiff() {
  // Default limits from the proposal: +-20 öre doesn't matter, +-80 öre is "very"
  check("levelForDiff: 0 öre is neutral", levelForDiff(0, 20, 80), LEVEL_NEUTRAL);
  check("levelForDiff: +19.9 öre is still neutral", levelForDiff(19.9f, 20, 80), LEVEL_NEUTRAL);
  check("levelForDiff: -19.9 öre is still neutral", levelForDiff(-19.9f, 20, 80), LEVEL_NEUTRAL);
  check("levelForDiff: +20 öre is expensive", levelForDiff(20, 20, 80), LEVEL_EXPENSIVE);
  check("levelForDiff: -20 öre is cheap", levelForDiff(-20, 20, 80), LEVEL_CHEAP);
  check("levelForDiff: +79.9 öre is expensive", levelForDiff(79.9f, 20, 80), LEVEL_EXPENSIVE);
  check("levelForDiff: -79.9 öre is cheap", levelForDiff(-79.9f, 20, 80), LEVEL_CHEAP);
  check("levelForDiff: +80 öre is very expensive", levelForDiff(80, 20, 80), LEVEL_VERY_EXPENSIVE);
  check("levelForDiff: -80 öre is very cheap", levelForDiff(-80, 20, 80), LEVEL_VERY_CHEAP);
  check("levelForDiff: +500 öre is very expensive", levelForDiff(500, 20, 80), LEVEL_VERY_EXPENSIVE);
  check("levelForDiff: other limits, +30 within +-50 is neutral", levelForDiff(30, 50, 150), LEVEL_NEUTRAL);
  check("levelForDiff: other limits, +150 is very expensive", levelForDiff(150, 50, 150), LEVEL_VERY_EXPENSIVE);

  // Levels never go down when the difference goes up
  bool ok = true;
  for (int d = -300; d < 300; d++) {
    if (levelForDiff((float)d + 1, 20, 80) < levelForDiff((float)d, 20, 80)) ok = false;
  }
  checkTrue("levelForDiff: never decreases from -300 to +300 öre", ok);
}

static void testLevelForPrice() {
  check("levelForPrice: normal price is neutral", levelForPrice(100, 100, 20, 80), LEVEL_NEUTRAL);
  check("levelForPrice: 30 öre under normal is cheap", levelForPrice(70, 100, 20, 80), LEVEL_CHEAP);
  check("levelForPrice: 100 öre over normal is very expensive", levelForPrice(200, 100, 20, 80), LEVEL_VERY_EXPENSIVE);
  check("levelForPrice: zero price is very cheap", levelForPrice(0, 100, 20, 80), LEVEL_VERY_CHEAP);
  check("levelForPrice: negative price is very cheap", levelForPrice(-5, 100, 20, 80), LEVEL_VERY_CHEAP);
  check("levelForPrice: zero price is very cheap even when the normal price is low", levelForPrice(0, 5, 20, 80), LEVEL_VERY_CHEAP);
  check("levelForPrice: positive price with a negative normal price can be expensive", levelForPrice(30, -10, 20, 80), LEVEL_EXPENSIVE);
  check("levelForPrice: a 30-40 öre day around a 35 öre normal is neutral", levelForPrice(40, 35, 20, 80), LEVEL_NEUTRAL);
}

static void testLevelHue() {
  check("levelHue 5 levels: very cheap is green (96)", levelHue(LEVEL_VERY_CHEAP, true), 96);
  check("levelHue 5 levels: cheap is yellow-green (80)", levelHue(LEVEL_CHEAP, true), 80);
  check("levelHue 5 levels: neutral is yellow (64)", levelHue(LEVEL_NEUTRAL, true), 64);
  check("levelHue 5 levels: expensive is orange (32)", levelHue(LEVEL_EXPENSIVE, true), 32);
  check("levelHue 5 levels: very expensive is red (0)", levelHue(LEVEL_VERY_EXPENSIVE, true), 0);
  check("levelHue 3 levels: very cheap is green", levelHue(LEVEL_VERY_CHEAP, false), 96);
  check("levelHue 3 levels: cheap is green", levelHue(LEVEL_CHEAP, false), 96);
  check("levelHue 3 levels: neutral is yellow", levelHue(LEVEL_NEUTRAL, false), 64);
  check("levelHue 3 levels: expensive is red", levelHue(LEVEL_EXPENSIVE, false), 0);
  check("levelHue 3 levels: very expensive is red", levelHue(LEVEL_VERY_EXPENSIVE, false), 0);
  check("levelHue: below very cheap is clamped", levelHue(-9, true), 96);
  check("levelHue: above very expensive is clamped", levelHue(9, true), 0);
}

static void testTomorrowLeds() {
  check("TOMORROW_PERIODS: two periods", TOMORROW_PERIOD_COUNT, 2);
  check("TOMORROW_PERIODS: day starts 08", TOMORROW_PERIODS[0].startHour, 8);
  check("TOMORROW_PERIODS: day ends 17", TOMORROW_PERIODS[0].endHour, 17);
  check("TOMORROW_PERIODS: evening starts 17", TOMORROW_PERIODS[1].startHour, 17);
  check("TOMORROW_PERIODS: evening ends 24", TOMORROW_PERIODS[1].endHour, 24);
  bool ok = true;
  for (int p = 0; p < TOMORROW_PERIOD_COUNT; p++) {
    const HourPeriod& h = TOMORROW_PERIODS[p];
    if (h.startHour < 0 || h.endHour > 24 || h.startHour >= h.endHour || !h.label[0]) ok = false;
  }
  checkTrue("TOMORROW_PERIODS: every period lies within the day and has a label", ok);
  check("TOMORROW_LEDS: one off LED plus one per period", TOMORROW_LEDS, 3);

  check("dayLedCount: normal-price mode, 100 LEDs leaves 97 for today", dayLedCount(100, true), 97);
  check("dayLedCount: other color mode uses all 100", dayLedCount(100, false), 100);
  check("dayLedCount: normal-price mode, 4 LEDs leaves 1 for today", dayLedCount(4, true), 1);
  check("dayLedCount: normal-price mode, 3 LEDs is too short for tomorrow", dayLedCount(3, true), 3);
  check("dayLedCount: normal-price mode, 1 LED", dayLedCount(1, true), 1);

  // Today's LEDs still cover the whole day without gaps, on all day lengths
  const int days[] = {92, 96, 100};
  const int counts[] = {4, 53, 100, 300};
  for (int w : days) {
    for (int n : counts) {
      int m = dayLedCount(n, true), end = 0;
      bool ok2 = true;
      for (int i = 0; i < m; i++) {
        int s0, s1;
        ledSlotRange(i, w, m, s0, s1);
        if (s0 > end || s1 > w) ok2 = false;
        if (s1 > end) end = s1;
      }
      if (end != w) ok2 = false;
      char name[96];
      snprintf(name, sizeof(name), "normal-price mode: %d LEDs cover a %d-slot day", n, w);
      checkTrue(name, ok2);
    }
  }
}

// ---------- Other logic ----------

static void testIsNightHour() {
  checkTrue("isNightHour 23-6: 23 is night", isNightHour(23, 23, 6));
  checkTrue("isNightHour 23-6: 0 is night", isNightHour(0, 23, 6));
  checkTrue("isNightHour 23-6: 5 is night", isNightHour(5, 23, 6));
  checkTrue("isNightHour 23-6: 6 is day", !isNightHour(6, 23, 6));
  checkTrue("isNightHour 23-6: 22 is day", !isNightHour(22, 23, 6));
  checkTrue("isNightHour 1-5: 1 is night", isNightHour(1, 1, 5));
  checkTrue("isNightHour 1-5: 5 is day", !isNightHour(5, 1, 5));
  checkTrue("isNightHour 1-5: 0 is day", !isNightHour(0, 1, 5));
  checkTrue("isNightHour 6-6: never night", !isNightHour(6, 6, 6));
}

static void testBackoff() {
  check("backoffMs: first failure 30 s", backoffMs(1), 30000);
  check("backoffMs: second failure 60 s", backoffMs(2), 60000);
  check("backoffMs: sixth failure 16 min", backoffMs(6), 960000);
  check("backoffMs: seventh failure capped at 30 min", backoffMs(7), 1800000);
  check("backoffMs: many failures stay at 30 min", backoffMs(100), 1800000);
  check("backoffMs: 0 is treated as the first failure", backoffMs(0), 30000);
}

static void testValidArea() {
  checkTrue("validArea: SE1", validArea("SE1"));
  checkTrue("validArea: SE4", validArea("SE4"));
  checkTrue("validArea: SE5 invalid", !validArea("SE5"));
  checkTrue("validArea: lower case invalid", !validArea("se3"));
  checkTrue("validArea: empty string invalid", !validArea(""));
  checkTrue("validArea: too long invalid", !validArea("SE33"));
}

static void testParseVersion() {
  int v[3];
  bool pre = false;
  checkTrue("parseVersion: v2.1.3 parses", parseVersion("v2.1.3", v));
  check("parseVersion: v2.1.3 major", v[0], 2);
  check("parseVersion: v2.1.3 minor", v[1], 1);
  check("parseVersion: v2.1.3 patch", v[2], 3);
  checkTrue("parseVersion: v2 parses", parseVersion("v2", v));
  checkTrue("parseVersion: dev is invalid", !parseVersion("dev", v));
  checkTrue("parseVersion: empty is invalid", !parseVersion("", v));
  checkTrue("parseVersion: v2. is invalid", !parseVersion("v2.", v));
  checkTrue("parseVersion: four parts is invalid", !parseVersion("v2.1.3.4", v));
  checkTrue("parseVersion: trailing text is invalid", !parseVersion("v2.1x", v));
  checkTrue("parseVersion: huge number is invalid", !parseVersion("v99999999999", v));
  checkTrue("parseVersion: -rc1 is invalid without pre", !parseVersion("v2.1-rc1", v));
  checkTrue("parseVersion: -rc1 parses with pre", parseVersion("v2.1-rc1", v, &pre) && pre);
  checkTrue("parseVersion: bare - is invalid", !parseVersion("v2.1-", v, &pre));
}

static void testVersions() {
  checkTrue("isNewerVersion: v2.1 > v2.0", isNewerVersion("v2.1", "v2.0"));
  checkTrue("isNewerVersion: v2.10 > v2.9", isNewerVersion("v2.10", "v2.9"));
  checkTrue("isNewerVersion: v3.0 > v2.9.9", isNewerVersion("v3.0", "v2.9.9"));
  checkTrue("isNewerVersion: v3.0 > v2.5", isNewerVersion("v3.0", "v2.5"));
  checkTrue("isNewerVersion: v3.1 > v3.0", isNewerVersion("v3.1", "v3.0"));
  checkTrue("isNewerVersion: v4.0 > v3.1", isNewerVersion("v4.0", "v3.1"));
  checkTrue("isNewerVersion: v2.0.1 > v2.0", isNewerVersion("v2.0.1", "v2.0"));
  checkTrue("isNewerVersion: 2.1 without v > v2.0", isNewerVersion("2.1", "v2.0"));
  checkTrue("isNewerVersion: same version is not newer", !isNewerVersion("v2.0", "v2.0"));
  checkTrue("isNewerVersion: v2.0 equals v2.0.0", !isNewerVersion("v2.0.0", "v2.0"));
  checkTrue("isNewerVersion: older is not newer", !isNewerVersion("v1.9", "v2.0"));
  checkTrue("isNewerVersion: dev build is never replaced", !isNewerVersion("v9.0", "dev"));
  checkTrue("isNewerVersion: garbage is never newer", !isNewerVersion("latest", "v2.0"));
  checkTrue("isNewerVersion: null is never newer", !isNewerVersion(nullptr, "v2.0"));
  checkTrue("isNewerVersion: release candidate is never installed", !isNewerVersion("v2.1-rc1", "v2.0"));
  checkTrue("isNewerVersion: v2.1 replaces v2.1-rc1", isNewerVersion("v2.1", "v2.1-rc1"));
  checkTrue("isNewerVersion: v2.2 replaces v2.1-rc1", isNewerVersion("v2.2", "v2.1-rc1"));
  checkTrue("isNewerVersion: v2.0 does not replace v2.1-rc1", !isNewerVersion("v2.0", "v2.1-rc1"));
}

int main() {
  useTimeZone(SWEDEN_TZ);   // same as the lamp's default

  testDaysFromCivil();
  testParseIso();
  testPriceSlots();
  testDayLengths();
  testLocalHourSlot();
  testTimeZones();
  testLedSlotRange();
  testLedTimeState();
  testPhysicalLed();
  testPercentToScale();
  testInitialPastPercent();
  testPriceSum();
  testLevelForDiff();
  testLevelForPrice();
  testLevelHue();
  testTomorrowLeds();
  testIsNightHour();
  testBackoff();
  testValidArea();
  testParseVersion();
  testVersions();

  printf("\n%d OK, %d FAIL\n", passed, failed);
  return failed ? 1 : 0;
}
