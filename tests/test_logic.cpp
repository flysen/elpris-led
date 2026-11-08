// Tests for elpris_led/elpris_logic.h that run on a regular computer: make test
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../elpris_led/elpris_logic.h"

static int passed = 0, failed = 0;

static void check(const char* name, long long actual, long long expected) {
  if (actual == expected) { passed++; printf("OK   %s\n", name); }
  else { failed++; printf("FAIL %s: got %lld, expected %lld\n", name, actual, expected); }
}

static void checkTrue(const char* name, bool cond) { check(name, cond ? 1 : 0, 1); }

static time_t utc(int Y, int M, int D, int h, int m) {
  struct tm t = {};
  t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D; t.tm_hour = h; t.tm_min = m;
  return timegm(&t);
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
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);   // same as the lamp's default
  tzset();

  testDaysFromCivil();
  testParseIso();
  testPriceSlots();
  testDayLengths();
  testLedSlotRange();
  testLedTimeState();
  testPhysicalLed();
  testIsNightHour();
  testBackoff();
  testValidArea();
  testParseVersion();
  testVersions();

  printf("\n%d OK, %d FAIL\n", passed, failed);
  return failed ? 1 : 0;
}
