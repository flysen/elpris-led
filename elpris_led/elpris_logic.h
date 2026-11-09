// Pure logic without Arduino dependencies, so it can be tested on a regular computer (see tests/).
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <time.h>

const int SLOT_SEC = 900;   // 15 minutes
const unsigned long BACKOFF_MIN_MS = 30UL * 1000UL;
const unsigned long BACKOFF_MAX_MS = 30UL * 60UL * 1000UL;

// Number of days since 1970-01-01 for a date
inline long long daysFromCivil(int y, int m, int d) {
  y -= (m <= 2);
  long long era = (y >= 0 ? y : y - 399) / 400;
  int yoe = (int)(y - era * 400);
  int mp  = (m > 2) ? m - 3 : m + 9;
  int doy = (153 * mp + 2) / 5 + d - 1;
  long long doe = yoe * 365LL + yoe / 4 - yoe / 100 + doy;
  return era * 146097LL + doe - 719468LL;
}

// Parses ISO 8601, e.g. "2026-11-05T00:00:00+01:00", to UTC seconds.
// Without an offset, or with Z, the time is treated as UTC.
inline bool parseIso(const char* s, time_t& out) {
  int Y, M, D, h, m, sec, oh = 0, om = 0;
  char sign = 0;
  int n = sscanf(s, "%d-%d-%dT%d:%d:%d%c%d:%d", &Y, &M, &D, &h, &m, &sec, &sign, &oh, &om);
  if (n < 6) return false;
  long long t = daysFromCivil(Y, M, D) * 86400LL + h * 3600LL + m * 60LL + sec;
  if (n >= 9 && (sign == '+' || sign == '-')) {
    long long off = oh * 3600LL + om * 60LL;
    t -= (sign == '+') ? off : -off;
  }
  out = (time_t)t;
  return true;
}

// Slots covered by the price interval [a, b). If the end is missing or invalid, one hour is assumed.
inline void priceSlots(time_t a, time_t b, int32_t& s0, int32_t& s1) {
  if (b <= a) b = a + 3600;
  s0 = (int32_t)(a / SLOT_SEC);
  s1 = (int32_t)((b + SLOT_SEC - 1) / SLOT_SEC);
}

// Slot for local midnight dayOffset days from now (time zone from TZ).
inline int32_t localMidnightSlot(time_t now, int dayOffset) {
  struct tm t;
  localtime_r(&now, &t);
  t.tm_mday += dayOffset;
  t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
  t.tm_isdst = -1;
  return (int32_t)(mktime(&t) / SLOT_SEC);
}

// Which slots in the window LED number i shows. The whole window is covered without gaps.
inline void ledSlotRange(int i, int winSlots, int numLeds, int& s0, int& s1) {
  s0 = (int)((long long)i * winSlots / numLeds);
  s1 = (int)((long long)(i + 1) * winSlots / numLeds);
  if (s1 <= s0) s1 = s0 + 1;
}

// Where an LED showing slots [s0, s1) is in time, relative to the current slot cur
enum LedTime { LED_PAST, LED_NOW, LED_FUTURE };
inline LedTime ledTimeState(int s0, int s1, int cur) {
  if (cur >= s1) return LED_PAST;     // everything this LED shows has passed
  if (cur >= s0) return LED_NOW;      // this LED shows the current 15 minutes
  return LED_FUTURE;
}

// Physical LED on the strip for position i in time order. With reverse, time runs from the
// last LED to the first, for strips mounted so that the data input is on the right.
inline int physicalLed(int i, int numLeds, bool reverse) {
  return reverse ? numLeds - 1 - i : i;
}

// Brightness scale (0-255, as used by FastLED's nscale8) for a percentage 0-100, rounded and clamped
inline uint8_t percentToScale(int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (uint8_t)((pct * 255 + 50) / 100);
}

// Night mode, also when the interval crosses midnight. start == end means never night.
inline bool isNightHour(int hour, int start, int end) {
  if (start > end) return hour >= start || hour < end;
  return hour >= start && hour < end;
}

// Wait before the next attempt: 30 s, doubled per failure, at most 30 min.
inline unsigned long backoffMs(int failStreak) {
  int shift = failStreak - 1;
  if (shift < 0) shift = 0;
  if (shift > 6) shift = 6;
  unsigned long d = BACKOFF_MIN_MS << shift;
  return d > BACKOFF_MAX_MS ? BACKOFF_MAX_MS : d;
}

// SE1-SE4 in upper case
inline bool validArea(const char* a) {
  return a[0] == 'S' && a[1] == 'E' && a[2] >= '1' && a[2] <= '4' && a[3] == '\0';
}

// Parses a version like "v2.1" or "2.1.3" into major, minor, patch. Missing parts are 0.
// A suffix such as "-rc1" is only accepted when pre is given, and then sets *pre.
// Returns false for anything else (e.g. "dev", "v2.", "v2.1.3.4").
inline bool parseVersion(const char* s, int v[3], bool* pre = nullptr) {
  v[0] = v[1] = v[2] = 0;
  if (pre) *pre = false;
  if (!s) return false;
  if (*s == 'v' || *s == 'V') s++;
  for (int i = 0; i < 3; i++) {
    if (*s < '0' || *s > '9') return false;
    int x = 0;
    while (*s >= '0' && *s <= '9') {
      if (x > 99999) return false;
      x = x * 10 + (*s - '0');
      s++;
    }
    v[i] = x;
    if (*s == '\0') return true;
    if (*s == '-' && s[1] != '\0' && pre) { *pre = true; return true; }
    if (*s != '.' || i == 2) return false;
    s++;
  }
  return false;
}

// True if candidate is a stable release that should replace current.
// - Release candidates ("v2.1-rc1") are never installed automatically.
// - A lamp running a release candidate gets the stable release with the same number.
// - A current version that can't be parsed (a local "dev" build) is never replaced.
inline bool isNewerVersion(const char* candidate, const char* current) {
  int a[3], b[3];
  bool currentPre = false;
  if (!parseVersion(candidate, a) || !parseVersion(current, b, &currentPre)) return false;
  for (int i = 0; i < 3; i++) {
    if (a[i] != b[i]) return a[i] > b[i];
  }
  return currentPre;
}
