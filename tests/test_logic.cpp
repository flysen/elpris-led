// Tester för elpris_led/elpris_logic.h som körs på en vanlig dator: make test
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../elpris_led/elpris_logic.h"

static int passed = 0, failed = 0;

static void check(const char* name, long long actual, long long expected) {
  if (actual == expected) { passed++; printf("OK   %s\n", name); }
  else { failed++; printf("FEL  %s: fick %lld, väntade %lld\n", name, actual, expected); }
}

static void checkTrue(const char* name, bool cond) { check(name, cond ? 1 : 0, 1); }

static time_t utc(int Y, int M, int D, int h, int m) {
  struct tm t = {};
  t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D; t.tm_hour = h; t.tm_min = m;
  return timegm(&t);
}

static void testDaysFromCivil() {
  check("daysFromCivil: 1970-01-01", daysFromCivil(1970, 1, 1), 0);
  check("daysFromCivil: 2000-03-01, efter skottdag", daysFromCivil(2000, 3, 1), 11017);
  check("daysFromCivil: 2026-11-05", daysFromCivil(2026, 11, 5), utc(2026, 11, 5, 0, 0) / 86400);
}

static void testParseIso() {
  time_t t = 0;
  checkTrue("parseIso: vintertid +01:00 tolkas", parseIso("2026-11-05T00:00:00+01:00", t));
  check("parseIso: vintertid +01:00 blir rätt UTC", t, utc(2026, 11, 4, 23, 0));
  parseIso("2026-07-01T12:00:00+02:00", t);
  check("parseIso: sommartid +02:00 blir rätt UTC", t, utc(2026, 7, 1, 10, 0));
  parseIso("2026-01-01T00:15:00Z", t);
  check("parseIso: Z tolkas som UTC", t, utc(2026, 1, 1, 0, 15));
  parseIso("2026-01-01T06:00:00", t);
  check("parseIso: utan offset tolkas som UTC", t, utc(2026, 1, 1, 6, 0));
  checkTrue("parseIso: skräp ger false", !parseIso("inte ett datum", t));
}

static void testPriceSlots() {
  time_t a = utc(2026, 11, 5, 10, 0);
  int32_t s0, s1;
  priceSlots(a, a + 900, s0, s1);
  check("priceSlots: 15 minuter ger 1 slot", s1 - s0, 1);
  check("priceSlots: rätt startslot", s0, a / SLOT_SEC);
  priceSlots(a, a + 3600, s0, s1);
  check("priceSlots: 1 timme ger 4 slots", s1 - s0, 4);
  priceSlots(a, a, s0, s1);
  check("priceSlots: saknat slut antas vara 1 timme", s1 - s0, 4);
}

static void testDayLengths() {
  time_t normal = utc(2026, 11, 5, 11, 0);     // 12:00 svensk tid
  check("localMidnightSlot: midnatt 2026-11-05 svensk tid",
        localMidnightSlot(normal, 0), utc(2026, 11, 4, 23, 0) / SLOT_SEC);
  check("dygnslängd: vanligt dygn 96 slots",
        localMidnightSlot(normal, 1) - localMidnightSlot(normal, 0), 96);

  time_t spring = utc(2026, 3, 29, 10, 0);
  check("dygnslängd: 2026-03-29, sommartid börjar, 92 slots",
        localMidnightSlot(spring, 1) - localMidnightSlot(spring, 0), 92);

  time_t autumn = utc(2026, 10, 25, 10, 0);
  check("dygnslängd: 2026-10-25, sommartid slutar, 100 slots",
        localMidnightSlot(autumn, 1) - localMidnightSlot(autumn, 0), 100);

  time_t dayBefore = utc(2026, 3, 28, 11, 0);
  check("fönster 48 h över sommartidsbytet: 188 slots",
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
        if (s0 > end) ok = false;            // lucka
        if (s1 > end) end = s1;
      }
      if (end != w) ok = false;
      char name[96];
      snprintf(name, sizeof(name), "ledSlotRange: %d LED över %d slots utan luckor", n, w);
      checkTrue(name, ok);
    }
  }
}

static void testIsNightHour() {
  checkTrue("isNightHour 23-6: 23 är natt", isNightHour(23, 23, 6));
  checkTrue("isNightHour 23-6: 0 är natt", isNightHour(0, 23, 6));
  checkTrue("isNightHour 23-6: 5 är natt", isNightHour(5, 23, 6));
  checkTrue("isNightHour 23-6: 6 är dag", !isNightHour(6, 23, 6));
  checkTrue("isNightHour 23-6: 22 är dag", !isNightHour(22, 23, 6));
  checkTrue("isNightHour 1-5: 1 är natt", isNightHour(1, 1, 5));
  checkTrue("isNightHour 1-5: 5 är dag", !isNightHour(5, 1, 5));
  checkTrue("isNightHour 1-5: 0 är dag", !isNightHour(0, 1, 5));
  checkTrue("isNightHour 6-6: aldrig natt", !isNightHour(6, 6, 6));
}

static void testBackoff() {
  check("backoffMs: första felet 30 s", backoffMs(1), 30000);
  check("backoffMs: andra felet 60 s", backoffMs(2), 60000);
  check("backoffMs: sjätte felet 16 min", backoffMs(6), 960000);
  check("backoffMs: sjunde felet tak 30 min", backoffMs(7), 1800000);
  check("backoffMs: många fel stannar på 30 min", backoffMs(100), 1800000);
  check("backoffMs: 0 behandlas som första felet", backoffMs(0), 30000);
}

static void testValidArea() {
  checkTrue("validArea: SE1", validArea("SE1"));
  checkTrue("validArea: SE4", validArea("SE4"));
  checkTrue("validArea: SE5 ogiltigt", !validArea("SE5"));
  checkTrue("validArea: gemener ogiltigt", !validArea("se3"));
  checkTrue("validArea: tom sträng ogiltig", !validArea(""));
  checkTrue("validArea: för lång ogiltig", !validArea("SE33"));
}

int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);   // samma som lampans standard
  tzset();

  testDaysFromCivil();
  testParseIso();
  testPriceSlots();
  testDayLengths();
  testLedSlotRange();
  testIsNightHour();
  testBackoff();
  testValidArea();

  printf("\n%d OK, %d FEL\n", passed, failed);
  return failed ? 1 : 0;
}
