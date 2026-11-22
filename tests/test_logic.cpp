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
  parseIso("2026-01-01T00:00:00-05:00", t);
  check("parseIso: negative offset -05:00 gives correct UTC", t, utc(2026, 1, 1, 5, 0));
  parseIso("2026-01-01T23:30:00-03:30", t);
  check("parseIso: negative offset with minutes, next day in UTC", t, utc(2026, 1, 2, 3, 0));
  parseIso("2026-06-01T12:00:00+05:30", t);
  check("parseIso: positive offset with minutes", t, utc(2026, 6, 1, 6, 30));
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

static void testNightModeActive() {
  checkTrue("nightModeActive: on, 23-6, at 2 is night", nightModeActive(true, 2, 23, 6));
  checkTrue("nightModeActive: on, 23-6, at 12 is day", !nightModeActive(true, 12, 23, 6));
  checkTrue("nightModeActive: off, 23-6, at 2 is day", !nightModeActive(false, 2, 23, 6));
  checkTrue("nightModeActive: off, 23-6, at 23 is day", !nightModeActive(false, 23, 23, 6));
  checkTrue("nightModeActive: on, 6-6 is never night", !nightModeActive(true, 6, 6, 6));
  bool ok = true;
  for (int h = 0; h < 24; h++) if (nightModeActive(false, h, 0, 23) || nightModeActive(false, h, 23, 6)) ok = false;
  checkTrue("nightModeActive: off is day at every hour", ok);
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

static void testResponseSnippet() {
  char out[64];
  const char* html = "<!DOCTYPE html>\n<html lang=\"en\">";
  responseSnippet(html, strlen(html), out, sizeof(out));
  checkTrue("responseSnippet: a line break becomes one space", strcmp(out, "<!DOCTYPE html> <html lang=\"en\">") == 0);

  const char* spaced = "  \r\n\t [ {\"a\": 1} ]  \n";
  responseSnippet(spaced, strlen(spaced), out, sizeof(out));
  checkTrue("responseSnippet: leading, trailing and repeated whitespace is dropped", strcmp(out, "[ {\"a\": 1} ]") == 0);

  char small[6];
  responseSnippet("abcdefghij", 10, small, sizeof(small));
  checkTrue("responseSnippet: cut to fit the buffer", strcmp(small, "abcde") == 0);

  responseSnippet("", 0, out, sizeof(out));
  checkTrue("responseSnippet: an empty response gives an empty string", out[0] == '\0');

  const char bin[] = {'O', 'K', 0x01, 0x7f, 'x'};
  responseSnippet(bin, sizeof(bin), out, sizeof(out));
  checkTrue("responseSnippet: control characters become a single space", strcmp(out, "OK x") == 0);
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
  checkTrue("isNewerVersion: v4.0.1 > v4.0", isNewerVersion("v4.0.1", "v4.0"));
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

// ---------- Update test channel ----------

static void testParseRelease() {
  int v[3], rc;
  checkTrue("parseRelease: v4.0 is a stable release", parseRelease("v4.0", v, rc) && rc == 0);
  checkTrue("parseRelease: v4.0-rc1 is release candidate 1 of 4.0",
            parseRelease("v4.0-rc1", v, rc) && rc == 1 && v[0] == 4 && v[1] == 0);
  checkTrue("parseRelease: v4.0-rc12 is release candidate 12", parseRelease("v4.0-rc12", v, rc) && rc == 12);
  checkTrue("parseRelease: -beta is not a release", !parseRelease("v4.0-beta", v, rc));
  checkTrue("parseRelease: -rc without a number is not a release", !parseRelease("v4.0-rc", v, rc));
  checkTrue("parseRelease: -rc0 is not a release", !parseRelease("v4.0-rc0", v, rc));
  checkTrue("parseRelease: text after -rc1 is not a release", !parseRelease("v4.0-rc1x", v, rc));
  checkTrue("parseRelease: dev is not a release", !parseRelease("dev", v, rc));
}

static void testTestChannel() {
  checkTrue("test channel: v4.0-rc1 replaces v3.2", isNewerTestVersion("v4.0-rc1", "v3.2"));
  checkTrue("test channel: v4.0-rc2 replaces v4.0-rc1", isNewerTestVersion("v4.0-rc2", "v4.0-rc1"));
  checkTrue("test channel: v4.0-rc10 replaces v4.0-rc9", isNewerTestVersion("v4.0-rc10", "v4.0-rc9"));
  checkTrue("test channel: v4.0 replaces v4.0-rc2", isNewerTestVersion("v4.0", "v4.0-rc2"));
  checkTrue("test channel: v4.1-rc1 replaces v4.0", isNewerTestVersion("v4.1-rc1", "v4.0"));
  checkTrue("test channel: v4.0-rc1 does not replace v4.0", !isNewerTestVersion("v4.0-rc1", "v4.0"));
  checkTrue("test channel: v4.0-rc1 does not replace v4.0-rc2", !isNewerTestVersion("v4.0-rc1", "v4.0-rc2"));
  checkTrue("test channel: the same version is not newer", !isNewerTestVersion("v4.0-rc1", "v4.0-rc1"));
  checkTrue("test channel: v3.1 does not replace v3.2", !isNewerTestVersion("v3.1", "v3.2"));
  checkTrue("test channel: a dev build is never replaced", !isNewerTestVersion("v9.0", "dev"));
  checkTrue("test channel: other suffixes are never installed", !isNewerTestVersion("v9.0-beta", "v4.0"));
  checkTrue("test channel: garbage is never newer", !isNewerTestVersion("latest", "v4.0"));

  int a[3] = {4, 0, 0}, b[3] = {4, 0, 0}, c[3] = {4, 1, 0};
  check("compareReleases: equal releases", compareReleases(a, 0, b, 0), 0);
  check("compareReleases: a stable release is newer than its rc", compareReleases(a, 0, b, 3), 1);
  check("compareReleases: an rc is older than its stable release", compareReleases(a, 3, b, 0), -1);
  check("compareReleases: the version number counts before rc", compareReleases(c, 1, a, 0), 1);

  // The stable channel still never installs release candidates
  checkTrue("stable channel: v4.0-rc1 is not installed from v3.2", !isNewerVersion("v4.0-rc1", "v3.2"));
}

// ---------- The board's blue status LED ----------

// The pattern as a string: "+" for each unit on, "." for each unit off, with the unit in ms
static void patternString(const BlinkStep* steps, int n, int unit, char* out, size_t size) {
  size_t o = 0;
  for (int i = 0; i < n && o + 1 < size; i++) {
    for (int k = 0; k < steps[i].ms / unit && o + 1 < size; k++) out[o++] = steps[i].on ? '+' : '.';
  }
  out[o] = '\0';
}

static void testStatusLedState() {
  check("statusLedState: the portal blinks fast", statusLedState(true, false, true, false, false, false), STATUS_LED_PORTAL);
  check("statusLedState: the portal blinks even with the LED turned off", statusLedState(true, false, false, false, false, false), STATUS_LED_PORTAL);
  check("statusLedState: connecting blinks fast", statusLedState(false, false, true, false, false, false), STATUS_LED_CONNECTING);
  check("statusLedState: connecting blinks even with the LED turned off", statusLedState(false, false, false, false, true, true), STATUS_LED_CONNECTING);
  check("statusLedState: turned off once connected", statusLedState(false, true, false, true, true, true), STATUS_LED_OFF);
  check("statusLedState: the address comes before fetching", statusLedState(false, true, true, true, true, false), STATUS_LED_ADDRESS);
  check("statusLedState: the address comes before a fetch error", statusLedState(false, true, true, true, false, true), STATUS_LED_ADDRESS);
  check("statusLedState: busy comes before a fetch error", statusLedState(false, true, true, false, true, true), STATUS_LED_BUSY);
  check("statusLedState: fetch errors", statusLedState(false, true, true, false, false, true), STATUS_LED_FAILING);
  check("statusLedState: everything works", statusLedState(false, true, true, false, false, false), STATUS_LED_OK);
}

static void testStatusLedPatterns() {
  const BlinkStep* steps;
  int n;
  statusLedPattern(STATUS_LED_PORTAL, steps, n);
  check("pattern portal: 100 ms on, 100 ms off", blinkPatternMs(steps, n), 200);
  checkTrue("pattern portal: on at 50 ms, off at 150 ms", blinkPatternOn(steps, n, 50) && !blinkPatternOn(steps, n, 150));
  statusLedPattern(STATUS_LED_CONNECTING, steps, n);
  check("pattern connecting: same as the portal", blinkPatternMs(steps, n), 200);
  statusLedPattern(STATUS_LED_BUSY, steps, n);
  checkTrue("pattern busy: always on", blinkPatternOn(steps, n, 0) && blinkPatternOn(steps, n, 999) && blinkPatternOn(steps, n, 123456));
  statusLedPattern(STATUS_LED_FAILING, steps, n);
  check("pattern failing: a double blink every 1.95 s", blinkPatternMs(steps, n), 1950);
  checkTrue("pattern failing: on, off, on, then a long pause",
            blinkPatternOn(steps, n, 100) && !blinkPatternOn(steps, n, 200) && blinkPatternOn(steps, n, 350) && !blinkPatternOn(steps, n, 1000));
  statusLedPattern(STATUS_LED_OK, steps, n);
  check("pattern ok: one blink every 10 s", blinkPatternMs(steps, n), 10000);
  checkTrue("pattern ok: on for 120 ms, then off", blinkPatternOn(steps, n, 100) && !blinkPatternOn(steps, n, 120) && !blinkPatternOn(steps, n, 9999));
  checkTrue("pattern ok: repeats", blinkPatternOn(steps, n, 20050));
  statusLedPattern(STATUS_LED_OFF, steps, n);
  checkTrue("pattern off: never on", !blinkPatternOn(steps, n, 0) && !blinkPatternOn(steps, n, 500));
  checkTrue("blinkPatternOn: an empty pattern is off", !blinkPatternOn(steps, 0, 0));
  const BlinkStep zero[] = {{1, 0}};
  checkTrue("blinkPatternOn: a pattern of length 0 is off", !blinkPatternOn(zero, 1, 0));
}

static void testAddressPartsToShow() {
  const uint8_t m24[4] = {255, 255, 255, 0}, m16[4] = {255, 255, 0, 0}, m8[4] = {255, 0, 0, 0};
  const uint8_t m20[4] = {255, 255, 240, 0}, m32[4] = {255, 255, 255, 255}, m0[4] = {0, 0, 0, 0};
  check("addressPartsToShow: /24 shows the last part", addressPartsToShow(m24), 1);
  check("addressPartsToShow: /16 shows the last two", addressPartsToShow(m16), 2);
  check("addressPartsToShow: /8 shows the last three", addressPartsToShow(m8), 3);
  check("addressPartsToShow: /20 shows the last two", addressPartsToShow(m20), 2);
  check("addressPartsToShow: /32 still shows one part", addressPartsToShow(m32), 1);
  check("addressPartsToShow: no mask shows the whole address", addressPartsToShow(m0), 4);
}

static void testAddressBlinkSteps() {
  BlinkStep steps[MAX_BLINK_STEPS];
  char s[600];

  // 172.16.1.90 on a /16 network: "1" and "90", as in the emulator
  const uint8_t ip[4] = {172, 16, 1, 90};
  int n = addressBlinkSteps(ip, 2, 100, steps, MAX_BLINK_STEPS);
  patternString(steps, n, 100, s, sizeof(s));
  checkTrue("addressBlinkSteps: 1.90 is 1 blink, long pause, 9 blinks, pause, one long blink, end pause",
            strcmp(s, "+..........."
                      "+.+.+.+.+.+.+.+.+.....++++."
                      "....................") == 0);
  check("addressBlinkSteps: 1.90 takes 5.9 s with 100 ms blinks", blinkPatternMs(steps, n), 5900);

  // /24 shows only the last part
  n = addressBlinkSteps(ip, 1, 100, steps, MAX_BLINK_STEPS);
  patternString(steps, n, 100, s, sizeof(s));
  checkTrue("addressBlinkSteps: 90 on a /24 network", strcmp(s, "+.+.+.+.+.+.+.+.+.....++++.....................") == 0);

  // Every digit is counted right: the number of blinks equals the digit, 0 is one long blink
  const uint8_t all[4] = {0, 9, 105, 255};
  n = addressBlinkSteps(all, 4, 250, steps, MAX_BLINK_STEPS);
  int blinks = 0, longBlinks = 0;
  for (int i = 0; i < n; i++) {
    if (!steps[i].on) continue;
    if (steps[i].ms == 250) blinks++;
    else if (steps[i].ms == 1000) longBlinks++;
  }
  check("addressBlinkSteps: 0.9.105.255 has 0+9+1+5+2+5+5 short blinks", blinks, 27);
  check("addressBlinkSteps: 0.9.105.255 has two long blinks (the zeros)", longBlinks, 2);
  checkTrue("addressBlinkSteps: the pattern ends off", !steps[n - 1].on);

  // The longest address fits in MAX_BLINK_STEPS
  const uint8_t longest[4] = {199, 199, 199, 199};
  n = addressBlinkSteps(longest, 4, 250, steps, MAX_BLINK_STEPS);
  checkTrue("addressBlinkSteps: 199.199.199.199 fits", n < MAX_BLINK_STEPS);
  checkTrue("addressBlinkSteps: every step fits in 16 bits at 250 ms", blinkPatternMs(steps, n) > 0 && steps[n - 1].ms == 5000);

  // A too small buffer is never overrun
  BlinkStep few[5];
  check("addressBlinkSteps: stops at the buffer size", addressBlinkSteps(longest, 4, 250, few, 5), 5);

  // Parts outside 1-4 are clamped
  n = addressBlinkSteps(ip, 0, 100, steps, MAX_BLINK_STEPS);
  int one = addressBlinkSteps(ip, 1, 100, steps + 128, 128);
  check("addressBlinkSteps: 0 parts is shown as 1", n, one);
  n = addressBlinkSteps(ip, 9, 100, steps, MAX_BLINK_STEPS);
  int four = addressBlinkSteps(ip, 4, 100, steps + 128, 128);
  check("addressBlinkSteps: 9 parts is shown as 4", n, four);
}

// ---------- Reading a day of prices ----------

// A file from tests/fixtures as a string (the tests run from the repository root)
static char* readFixture(const char* name, size_t& len) {
  char path[256];
  snprintf(path, sizeof(path), "tests/fixtures/%s", name);
  FILE* f = fopen(path, "rb");
  if (!f) { len = 0; return nullptr; }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*)malloc((size_t)size + 1);
  len = fread(buf, 1, (size_t)size, f);
  buf[len] = '\0';
  fclose(f);
  return buf;
}

// A day in the elprisetjustnu.se format, counted in UTC from the day's local midnight: periods of the given
// minutes, with the UTC offset (in minutes) changing at changeUtc, and prices 0.10, 0.11, ... in SEK
static void buildDay(char* out, size_t size, time_t midnightUtc, int periods, int minutes,
                     time_t changeUtc, int offBefore, int offAfter) {
  size_t o = 0;
  o += snprintf(out + o, size - o, "[");
  // Local time with its offset, e.g. "2026-10-25T02:45:00+02:00", for UTC time t and offset off (minutes)
  auto local = [](time_t t, int off, char* buf, size_t n) {
    time_t lt = t + off * 60;
    struct tm g;
    gmtime_r(&lt, &g);
    snprintf(buf, n, "%04d-%02d-%02dT%02d:%02d:00%c%02d:%02d", g.tm_year + 1900, g.tm_mon + 1, g.tm_mday,
             g.tm_hour, g.tm_min, off < 0 ? '-' : '+', abs(off) / 60 % 100, abs(off) % 60);
  };
  for (int i = 0; i < periods; i++) {
    time_t a = midnightUtc + (time_t)i * minutes * 60, b = a + minutes * 60;
    char sa[80], sb[80];
    local(a, a < changeUtc ? offBefore : offAfter, sa, sizeof(sa));
    local(b, (b - 1) < changeUtc ? offBefore : offAfter, sb, sizeof(sb));   // the end in the offset just before it
    o += snprintf(out + o, size - o,
                  "%s{\"SEK_per_kWh\":%.2f,\"EUR_per_kWh\":0.01,\"EXR\":11.0,\"time_start\":\"%s\",\"time_end\":\"%s\"}",
                  i ? "," : "", 0.10 + i * 0.01, sa, sb);
  }
  snprintf(out + o, size - o, "]");
}

static void testParsePricesRealDay() {
  size_t len;
  char* body = readFixture("prices-2026-11-22-SE3.json", len);
  checkTrue("parsePrices: the fixture with a real day can be read", body != nullptr && len > 13000);
  if (!body) return;
  PriceEntry e[MAX_PRICE_ENTRIES];
  bool complete;
  int n = parsePrices(body, len, e, MAX_PRICE_ENTRIES, complete);
  check("parsePrices: a real day has 96 prices", n, 96);
  checkTrue("parsePrices: a real day is complete", complete);
  check("parsePrices: first price starts at local midnight", e[0].start, utc(2026, 11, 21, 23, 0));
  check("parsePrices: first price lasts 15 minutes", e[0].end - e[0].start, 900);
  checkNear("parsePrices: first price is 0.95908 SEK", e[0].price, 0.95908);
  checkNear("parsePrices: last price is 0.98377 SEK", e[95].price, 0.98377);
  check("parsePrices: last price ends at the next local midnight", e[95].end, utc(2026, 11, 22, 23, 0));
  bool contiguous = true;
  for (int i = 1; i < n; i++) if (e[i].start != e[i - 1].end) contiguous = false;
  checkTrue("parsePrices: the periods follow each other without gaps", contiguous);

  // Each period fills exactly one quarter, and together they fill the day
  int32_t s0, s1, first = 0, last = 0, total = 0;
  for (int i = 0; i < n; i++) {
    priceSlots(e[i].start, e[i].end, s0, s1);
    if (i == 0) first = s0;
    last = s1;
    total += s1 - s0;
  }
  check("parsePrices: the day starts at the local midnight slot", first, localMidnightSlot(utc(2026, 11, 22, 12, 0), 0));
  check("parsePrices: the day ends at the next midnight slot", last, localMidnightSlot(utc(2026, 11, 22, 12, 0), 1));
  check("parsePrices: 96 quarters in all", total, 96);

  // What the lamp read in v5.0-v5.1: only the first 571 bytes
  n = parsePrices(body, 571, e, MAX_PRICE_ENTRIES, complete);
  check("parsePrices: cut off after 571 bytes gives the 4 whole prices before the cut", n, 4);
  checkTrue("parsePrices: cut off after 571 bytes is not complete", !complete);
  // Cut off at every length: never more prices than fit, never complete before the end, never a crash
  bool ok = true;
  int prev = 0;
  for (size_t cut = 0; cut < len; cut += 37) {
    int m = parsePrices(body, cut, e, MAX_PRICE_ENTRIES, complete);
    if (complete || m < prev || m > 96) ok = false;
    prev = m;
  }
  checkTrue("parsePrices: cut off anywhere, a day is never complete and the count only grows", ok);
  n = parsePrices(body, len - 1, e, MAX_PRICE_ENTRIES, complete);
  checkTrue("parsePrices: missing only the final ] is not complete", n == 96 && !complete);

  // More prices than room: the first ones are kept
  n = parsePrices(body, len, e, 10, complete);
  checkTrue("parsePrices: stops at the room given", n == 10 && complete);
  free(body);
}

static void testParsePricesDst() {
  static char body[40000];
  PriceEntry e[MAX_PRICE_ENTRIES];
  bool complete;
  int32_t s0, s1;

  // 2026-03-29: clocks go forward at 02:00, 23 hours, 92 quarters
  buildDay(body, sizeof(body), utc(2026, 3, 28, 23, 0), 92, 15, utc(2026, 3, 29, 1, 0), 60, 120);
  int n = parsePrices(body, strlen(body), e, MAX_PRICE_ENTRIES, complete);
  check("parsePrices: 2026-03-29 has 92 prices", n, 92);
  int total = 0;
  for (int i = 0; i < n; i++) { priceSlots(e[i].start, e[i].end, s0, s1); total += s1 - s0; }
  check("parsePrices: 2026-03-29 fills 92 quarters", total, 92);
  check("parsePrices: 2026-03-29 first price at local midnight", e[0].start, utc(2026, 3, 28, 23, 0));
  check("parsePrices: 2026-03-29 03:00 local is 01:00 UTC", e[8].start, utc(2026, 3, 29, 1, 0));

  // 2026-10-25: clocks go back at 03:00, 25 hours, 100 quarters
  buildDay(body, sizeof(body), utc(2026, 10, 24, 22, 0), 100, 15, utc(2026, 10, 25, 1, 0), 120, 60);
  n = parsePrices(body, strlen(body), e, MAX_PRICE_ENTRIES, complete);
  check("parsePrices: 2026-10-25 has 100 prices", n, 100);
  total = 0;
  bool contiguous = true;
  for (int i = 0; i < n; i++) {
    priceSlots(e[i].start, e[i].end, s0, s1);
    total += s1 - s0;
    if (i && e[i].start != e[i - 1].end) contiguous = false;
  }
  check("parsePrices: 2026-10-25 fills 100 quarters", total, 100);
  checkTrue("parsePrices: 2026-10-25 has no gaps or overlaps across the repeated hour", contiguous);
  checkTrue("parsePrices: 2026-10-25 is complete", complete);

  // Hourly prices, as the API gave before 15-minute prices: 24 periods of 4 quarters
  buildDay(body, sizeof(body), utc(2026, 1, 14, 23, 0), 24, 60, utc(2030, 1, 1, 0, 0), 60, 60);
  n = parsePrices(body, strlen(body), e, MAX_PRICE_ENTRIES, complete);
  check("parsePrices: an hourly day has 24 prices", n, 24);
  priceSlots(e[0].start, e[0].end, s0, s1);
  check("parsePrices: an hourly price fills 4 quarters", s1 - s0, 4);
}

static void testParsePricesOdd() {
  PriceEntry e[8];
  bool complete;
  const char* html = "<!DOCTYPE html><html><body>Bad gateway</body></html>";
  check("parsePrices: an HTML error page gives no prices", parsePrices(html, strlen(html), e, 8, complete), 0);
  checkTrue("parsePrices: an HTML error page is not complete", !complete);
  check("parsePrices: an empty answer gives no prices", parsePrices("", 0, e, 8, complete), 0);
  check("parsePrices: null gives no prices", parsePrices(nullptr, 5, e, 8, complete), 0);
  check("parsePrices: [] is an empty, complete day", parsePrices("[]", 2, e, 8, complete) + (complete ? 100 : 0), 100);
  check("parsePrices: [ ] with spaces is too", parsePrices(" [ \n ] ", 7, e, 8, complete) + (complete ? 100 : 0), 100);

  const char* pretty = "[\n  {\n    \"time_start\" : \"2026-11-22T00:00:00+01:00\",\n    \"SEK_per_kWh\" : -0.0123,\n"
                       "    \"time_end\" : \"2026-11-22T00:15:00+01:00\"\n  } ,\n  {\"SEK_per_kWh\":1.5e-1,"
                       "\"time_start\":\"2026-11-22T00:15:00+01:00\",\"time_end\":\"2026-11-22T00:30:00+01:00\"}\n]\n";
  int n = parsePrices(pretty, strlen(pretty), e, 8, complete);
  checkTrue("parsePrices: spaces, line breaks and another key order are fine", n == 2 && complete);
  checkNear("parsePrices: a negative price", e[0].price, -0.0123);
  checkNear("parsePrices: a price written with an exponent", e[1].price, 0.15);

  const char* noEnd = "[{\"SEK_per_kWh\":0.5,\"time_start\":\"2026-11-22T00:00:00+01:00\"}]";
  n = parsePrices(noEnd, strlen(noEnd), e, 8, complete);
  checkTrue("parsePrices: a missing time_end gives end = start (priceSlots then assumes an hour)", n == 1 && e[0].end == e[0].start);

  const char* gaps = "[{\"time_start\":\"2026-11-22T00:00:00+01:00\"},"
                     "{\"SEK_per_kWh\":0.5},"
                     "{\"SEK_per_kWh\":\"x\",\"time_start\":\"2026-11-22T00:30:00+01:00\"},"
                     "{\"SEK_per_kWh\":0.7,\"time_start\":\"not a date\"},"
                     "{\"SEK_per_kWh\":0.8,\"time_start\":\"2026-11-22T00:45:00+01:00\",\"time_end\":\"2026-11-22T01:00:00+01:00\"}]";
  n = parsePrices(gaps, strlen(gaps), e, 8, complete);
  checkTrue("parsePrices: periods without a price or a start time are skipped", n == 1 && complete);
  checkNear("parsePrices: the good period is kept", e[0].price, 0.8);

  const char* nested = "[{\"SEK_per_kWh\":0.5,\"extra\":{\"a\":1},\"time_start\":\"2026-11-22T00:00:00+01:00\"}]";
  checkTrue("parsePrices: a nested object is treated as broken", parsePrices(nested, strlen(nested), e, 8, complete) == 0 && !complete);
  const char* braceInString = "[{\"note\":\"a } b\",\"SEK_per_kWh\":0.5,\"time_start\":\"2026-11-22T00:00:00+01:00\"}]";
  checkTrue("parsePrices: a } inside a string doesn't end the object", parsePrices(braceInString, strlen(braceInString), e, 8, complete) == 1 && complete);
  const char* junk = "[{\"SEK_per_kWh\":0.5,\"time_start\":\"2026-11-22T00:00:00+01:00\"} x]";
  checkTrue("parsePrices: junk after a period stops reading, not complete", parsePrices(junk, strlen(junk), e, 8, complete) == 1 && !complete);
  const char* notObject = "[1,2]";
  checkTrue("parsePrices: an array of numbers gives nothing", parsePrices(notObject, strlen(notObject), e, 8, complete) == 0 && !complete);
  const char* longTime = "[{\"SEK_per_kWh\":0.5,\"time_start\":\"2026-11-22T00:00:00+01:00 and much more text that does not fit\"}]";
  checkTrue("parsePrices: an over-long time string is cut, not overrun", parsePrices(longTime, strlen(longTime), e, 8, complete) <= 1);
  const char* openString = "[{\"SEK_per_kWh\":0.5,\"time_start\":\"2026-11-22T00:00";
  checkTrue("parsePrices: cut off inside a string", parsePrices(openString, strlen(openString), e, 8, complete) == 0 && !complete);
  const char* keyNoColon = "[{\"SEK_per_kWh\" 0.5,\"time_start\":\"2026-11-22T00:00:00+01:00\"}]";
  checkTrue("parsePrices: a key without a colon is not a value", parsePrices(keyNoColon, strlen(keyNoColon), e, 8, complete) == 0 && complete);
  const char* tabs = "\t[\r\n{\"SEK_per_kWh\"\t:\r\n0.5,\"time_start\" \t: \"2026-11-22T00:00:00+01:00\"}\r\n]";
  checkTrue("parsePrices: tabs and carriage returns are spaces too", parsePrices(tabs, strlen(tabs), e, 8, complete) == 1 && complete);
  const char* arrayValue = "[{\"SEK_per_kWh\":0.5,\"list\":[1],\"time_start\":\"2026-11-22T00:00:00+01:00\"}]";
  checkTrue("parsePrices: an array inside a period is treated as broken", parsePrices(arrayValue, strlen(arrayValue), e, 8, complete) == 0 && !complete);
  checkTrue("parsePrices: only [ is cut off", parsePrices("[", 1, e, 8, complete) == 0 && !complete);
  checkTrue("parsePrices: [ followed by a space only is cut off", parsePrices("[ ", 2, e, 8, complete) == 0 && !complete);
}

static void testJsonHelpers() {
  const char* obj = "{\"SEK_per_kWhX\":1,\"SEK_per_kWh\" \t:\r\n 2.5}";
  const char* end = obj + strlen(obj);
  const char* v = jsonValueOf(obj, end, "SEK_per_kWh");
  checkTrue("jsonValueOf: a longer key with the same start is not a match", v && *v == '2');
  checkTrue("jsonValueOf: a missing key gives null", jsonValueOf(obj, end, "time_start") == nullptr);
  const char* atEnd = "{\"SEK_per_kWh\"";
  checkTrue("jsonValueOf: a key at the very end gives null", jsonValueOf(atEnd, atEnd + strlen(atEnd), "SEK_per_kWh") == nullptr);
  const char* spaceThenEnd = "{\"SEK_per_kWh\"  ";
  checkTrue("jsonValueOf: a key followed only by spaces gives null", jsonValueOf(spaceThenEnd, spaceThenEnd + strlen(spaceThenEnd), "SEK_per_kWh") == nullptr);

  char buf[8];
  const char* str = "\"abcdefghij\"";
  checkTrue("jsonStringAt: a string longer than the room is cut", jsonStringAt(str, str + strlen(str), buf, sizeof(buf)) && strcmp(buf, "abcdefg") == 0);
  checkTrue("jsonStringAt: null is not a string", !jsonStringAt(nullptr, str, buf, sizeof(buf)));
  checkTrue("jsonStringAt: at the end is not a string", !jsonStringAt(str + strlen(str), str + strlen(str), buf, sizeof(buf)));
  checkTrue("jsonStringAt: a number is not a string", !jsonStringAt("12", str, buf, sizeof(buf)));
  checkTrue("jsonStringAt: no room is not a string", !jsonStringAt(str, str + strlen(str), buf, 0));
  checkTrue("jsonStringAt: no closing quote is not a string", !jsonStringAt(str, str + 5, buf, sizeof(buf)));
}

// ---------- Reading HTTP answers and firmware downloads ----------

static void testHttpBodyComplete() {
  checkTrue("httpBodyComplete: all of the given length arrived", httpBodyComplete(13607, 13607, false));
  checkTrue("httpBodyComplete: more than the given length is complete", httpBodyComplete(100, 120, false));
  checkTrue("httpBodyComplete: less than the given length is not, even if closed", !httpBodyComplete(13607, 571, true));
  checkTrue("httpBodyComplete: a length of 0 with nothing is complete", httpBodyComplete(0, 0, false));
  checkTrue("httpBodyComplete: no length, closed by the server is complete", httpBodyComplete(-1, 13607, true));
  checkTrue("httpBodyComplete: no length, gave up waiting is not", !httpBodyComplete(-1, 13607, false));
}

static void testFirmwareChecks() {
  const uint8_t good[] = {0xE9, 0x06, 0x02, 0x2f};
  const uint8_t html[] = {'<', '!', 'D', 'O'};
  checkTrue("firmwareHeaderOk: ESP32 firmware starts with 0xE9", firmwareHeaderOk(good, sizeof(good)));
  checkTrue("firmwareHeaderOk: an HTML page is not firmware", !firmwareHeaderOk(html, sizeof(html)));
  checkTrue("firmwareHeaderOk: nothing read yet is not firmware", !firmwareHeaderOk(good, 0));
  checkTrue("firmwareHeaderOk: no buffer is not firmware", !firmwareHeaderOk(nullptr, 4));

  const long slot = 1966080;   // the Minimal SPIFFS app slot
  checkTrue("firmwareSizeProblem: v5.1.1's 1331840 bytes fit", firmwareSizeProblem(1331840, slot) == nullptr);
  checkTrue("firmwareSizeProblem: exactly the slot fits", firmwareSizeProblem(slot, slot) == nullptr);
  checkTrue("firmwareSizeProblem: bigger than the slot", firmwareSizeProblem(slot + 1, slot) != nullptr);
  checkTrue("firmwareSizeProblem: no size given", firmwareSizeProblem(-1, slot) != nullptr);
  checkTrue("firmwareSizeProblem: size 0", firmwareSizeProblem(0, slot) != nullptr);
  checkTrue("firmwareSizeProblem: a tiny file is not firmware", firmwareSizeProblem(1200, slot) != nullptr);
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
  testNightModeActive();
  testBackoff();
  testValidArea();
  testParsePricesRealDay();
  testParsePricesDst();
  testParsePricesOdd();
  testJsonHelpers();
  testHttpBodyComplete();
  testFirmwareChecks();
  testResponseSnippet();
  testParseVersion();
  testVersions();
  testParseRelease();
  testTestChannel();
  testStatusLedState();
  testStatusLedPatterns();
  testAddressPartsToShow();
  testAddressBlinkSteps();

  printf("\n%d OK, %d FAIL\n", passed, failed);
  return failed ? 1 : 0;
}
