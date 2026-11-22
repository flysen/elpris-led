// Pure logic without Arduino dependencies, so it can be tested on a regular computer (see tests/).
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const int SLOT_SEC = 900;   // 15 minutes
const unsigned long BACKOFF_MIN_MS = 30UL * 1000UL;
const unsigned long BACKOFF_MAX_MS = 30UL * 60UL * 1000UL;

// Time zones offered in the web UI. The value is a POSIX TZ rule, which includes daylight saving time.
// The first entry is the default. Every rule is checked by the tests (offsets and DST change days).
struct TimeZoneChoice { const char* label; const char* tz; };
const TimeZoneChoice TIME_ZONES[] = {
  {"Sweden, Norway, Denmark, Central Europe", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Finland, Estonia, Latvia, Lithuania",     "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"United Kingdom, Ireland",                 "GMT0BST,M3.5.0/1,M10.5.0"},
  {"UTC (no daylight saving time)",           "UTC0"},
};
const int TIME_ZONE_COUNT = sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0]);

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

// Slot for local time hour:00 on the day dayOffset days from now (time zone from TZ). Hour 24 is the
// next midnight. Counted from local time, so periods come out right on 23- and 25-hour days.
inline int32_t localHourSlot(time_t now, int dayOffset, int hour) {
  struct tm t;
  localtime_r(&now, &t);
  t.tm_mday += dayOffset;
  t.tm_hour = hour; t.tm_min = 0; t.tm_sec = 0;
  t.tm_isdst = -1;
  return (int32_t)(mktime(&t) / SLOT_SEC);
}

// Slot for local midnight dayOffset days from now (time zone from TZ).
inline int32_t localMidnightSlot(time_t now, int dayOffset) {
  return localHourSlot(now, dayOffset, 0);
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

// Brightness of passed time when the settings are loaded. A saved percentage always wins. Without one
// (a lamp coming from v3.0), a lamp that had dimming turned off keeps full brightness, others get the default.
inline int initialPastPercent(bool hasSaved, int saved, bool oldDimOn, int defaultPct) {
  if (hasSaved) return saved;
  return oldDimOn ? defaultPct : 100;
}

// =====================================================================
//  Normal-price color mode (designed by @sealjo, see docs/normal-price-mode.md)
//
//  Today fills the strip from midnight to midnight, colored by how each price compares with a normal
//  price (the average of the last few days, today and, optionally, tomorrow). Within a neutral zone
//  the LED is yellow: it doesn't matter when you run the dishwasher. The last three LEDs are one off
//  LED and two LEDs comparing tomorrow's 08-17 and 17-24 with the same hours today.
// =====================================================================

// Sum and count of 15-minute prices, for averages over one or more days. Prices are in SEK/kWh.
struct PriceSum {
  double sum = 0;
  long count = 0;
  void add(float price, long quarters = 1) { sum += (double)price * quarters; count += quarters; }
  void add(const PriceSum& other) { sum += other.sum; count += other.count; }
  bool empty() const { return count == 0; }
  float mean() const { return count ? (float)(sum / count) : 0.0f; }
};

// One past day in the normal price: only the sum of its prices is kept, not every quarter.
// start is the day's local midnight slot, -1 for an empty entry. Defined here rather than in the sketch,
// because the Arduino builder declares the sketch's functions before types defined in the sketch.
struct HistoryDay { int32_t start = -1; PriceSum prices; };

// Price level: negative is cheaper, positive more expensive. 0 = within the neutral zone.
const int LEVEL_VERY_CHEAP     = -2;
const int LEVEL_CHEAP          = -1;
const int LEVEL_NEUTRAL        = 0;
const int LEVEL_EXPENSIVE      = 1;
const int LEVEL_VERY_EXPENSIVE = 2;

// Level for a difference in öre: within +-neutralOre it doesn't matter, from +-strongOre it is "very"
inline int levelForDiff(float diffOre, int neutralOre, int strongOre) {
  if (diffOre >= strongOre)   return LEVEL_VERY_EXPENSIVE;
  if (diffOre <= -strongOre)  return LEVEL_VERY_CHEAP;
  if (diffOre >= neutralOre)  return LEVEL_EXPENSIVE;
  if (diffOre <= -neutralOre) return LEVEL_CHEAP;
  return LEVEL_NEUTRAL;
}

// Level for a price compared with the normal price, both in öre. A price of zero or less is always very cheap.
inline int levelForPrice(float priceOre, float normalOre, int neutralOre, int strongOre) {
  if (priceOre <= 0) return LEVEL_VERY_CHEAP;
  return levelForDiff(priceOre - normalOre, neutralOre, strongOre);
}

// FastLED rainbow hue for a level. 5 levels: green, yellow-green, yellow, orange, red.
// 3 levels: green, yellow, red, with "very" merged into cheap and expensive.
inline uint8_t levelHue(int level, bool fiveLevels) {
  if (level < LEVEL_VERY_CHEAP) level = LEVEL_VERY_CHEAP;
  if (level > LEVEL_VERY_EXPENSIVE) level = LEVEL_VERY_EXPENSIVE;
  if (!fiveLevels) return level < 0 ? 96 : level > 0 ? 0 : 64;
  static const uint8_t hues[5] = {96, 80, 64, 32, 0};
  return hues[level + 2];
}

// Tomorrow's periods shown on the last LEDs, in local hours [start, end)
struct HourPeriod { int startHour; int endHour; const char* label; };
const HourPeriod TOMORROW_PERIODS[] = {
  {8,  17, "08-17"},
  {17, 24, "17-24"},
};
const int TOMORROW_PERIOD_COUNT = sizeof(TOMORROW_PERIODS) / sizeof(TOMORROW_PERIODS[0]);
const int TOMORROW_LEDS = 1 + TOMORROW_PERIOD_COUNT;   // one off LED, then one per period

// Number of LEDs that show today's timeline. In normal-price mode the last TOMORROW_LEDS are used for
// tomorrow, as long as the strip is long enough to keep at least one LED for today.
inline int dayLedCount(int numLeds, bool normalMode) {
  return (normalMode && numLeds > TOMORROW_LEDS) ? numLeds - TOMORROW_LEDS : numLeds;
}

// =====================================================================

// Night mode, also when the interval crosses midnight. start == end means never night.
inline bool isNightHour(int hour, int start, int end) {
  if (start > end) return hour >= start || hour < end;
  return hour >= start && hour < end;
}

// Whether the night brightness is used: only when night mode is turned on and the hour is within it.
// With night mode off the day brightness is used around the clock.
inline bool nightModeActive(bool enabled, int hour, int start, int end) {
  return enabled && isNightHour(hour, start, end);
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

// The first bytes of an unexpected server response, made readable on one line for the web UI: control
// characters become spaces, runs of spaces are collapsed, leading and trailing spaces are dropped, and the
// result is cut to fit outSize (including the terminating zero).
inline void responseSnippet(const char* in, size_t len, char* out, size_t outSize) {
  if (outSize == 0) return;
  size_t o = 0;
  bool lastSpace = true;   // drops leading spaces
  for (size_t i = 0; i < len && o + 1 < outSize; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c < 32 || c == 127) c = ' ';
    if (c == ' ') {
      if (lastSpace) continue;
      lastSpace = true;
    } else {
      lastSpace = false;
    }
    out[o++] = (char)c;
  }
  while (o > 0 && out[o - 1] == ' ') o--;
  out[o] = '\0';
}

// =====================================================================
//  Versions and update channels
// =====================================================================

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

// Stable channel: true if candidate is a stable release that should replace current.
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

// Parses a release tag: a stable version (rc = 0) or a release candidate "-rcN" with N from 1 (rc = N).
// Returns false for anything else, including other suffixes such as "-beta".
inline bool parseRelease(const char* s, int v[3], int& rc) {
  bool pre = false;
  rc = 0;
  if (!parseVersion(s, v, &pre)) return false;
  if (!pre) return true;
  const char* d = strchr(s, '-');
  if (!d || strncmp(d, "-rc", 3) != 0) return false;
  d += 3;
  if (*d < '1' || *d > '9') return false;
  int n = 0;
  while (*d >= '0' && *d <= '9') {
    if (n > 9999) return false;
    n = n * 10 + (*d - '0');
    d++;
  }
  if (*d != '\0') return false;
  rc = n;
  return true;
}

// Orders two releases: -1, 0 or 1. A stable release is newer than its own release candidates,
// and rc2 is newer than rc1. Version numbers count first: v4.1-rc1 is newer than v4.0.
inline int compareReleases(const int a[3], int aRc, const int b[3], int bRc) {
  for (int i = 0; i < 3; i++) {
    if (a[i] != b[i]) return a[i] > b[i] ? 1 : -1;
  }
  if (aRc == bRc) return 0;
  if (aRc == 0) return 1;
  if (bRc == 0) return -1;
  return aRc > bRc ? 1 : -1;
}

// Test channel: true if candidate, a stable release or a release candidate, should replace current.
// A current version that can't be parsed (a local "dev" build) is never replaced.
inline bool isNewerTestVersion(const char* candidate, const char* current) {
  int a[3], b[3], aRc, bRc;
  if (!parseRelease(candidate, a, aRc) || !parseRelease(current, b, bRc)) return false;
  return compareReleases(a, aRc, b, bRc) > 0;
}

// =====================================================================
//  The board's blue status LED (GPIO2 on ESP32 DevKit boards)
//
//  The same patterns and timings as the emulator's preview (docs/emulator/index.html), so the lamp
//  can be read the way the emulator shows it:
//  - fast blinking while connecting to wifi and while the lamp's own wifi Elpris-LED-Config is open
//  - the lamp's network address, three times after connecting: each digit as that many short blinks,
//    0 as one long blink, a short pause between digits and a long pause between the parts
//  - on while fetching prices or installing an update
//  - double blinks while fetching prices fails
//  - a short blink every 10 seconds when everything works
// =====================================================================

// One step of a blink pattern: on or off for a number of milliseconds. A pattern repeats.
struct BlinkStep { uint8_t on; uint16_t ms; };

const int BLINK_UNIT_MS   = 250;   // a short blink in the address
const int ADDRESS_REPEATS = 3;     // the address is shown this many times after connecting
const int MAX_BLINK_STEPS = 256;   // enough for a whole address: 12 digits of up to 9 blinks, plus pauses

enum StatusLed {
  STATUS_LED_OFF,         // turned off in the web UI
  STATUS_LED_CONNECTING,  // fast blinking
  STATUS_LED_PORTAL,      // fast blinking: the lamp's own wifi is open for setup
  STATUS_LED_ADDRESS,     // the network address
  STATUS_LED_BUSY,        // on: fetching prices or installing an update
  STATUS_LED_FAILING,     // double blinks
  STATUS_LED_OK,          // a short blink every 10 seconds
};

// What the LED shows. Setting up wifi always blinks, so a lamp can be set up even with the LED turned off;
// the address is shown in full before anything else, so its blinks can be counted.
inline StatusLed statusLedState(bool portal, bool connected, bool enabled, bool showingAddress, bool busy,
                                bool failing) {
  if (portal) return STATUS_LED_PORTAL;
  if (!connected) return STATUS_LED_CONNECTING;
  if (!enabled) return STATUS_LED_OFF;
  if (showingAddress) return STATUS_LED_ADDRESS;
  if (busy) return STATUS_LED_BUSY;
  if (failing) return STATUS_LED_FAILING;
  return STATUS_LED_OK;
}

// The fixed patterns. The address has its own, built by addressBlinkSteps().
inline void statusLedPattern(StatusLed state, const BlinkStep*& steps, int& count) {
  static const BlinkStep off[]     = {{0, 1000}};
  static const BlinkStep fast[]    = {{1, 100}, {0, 100}};
  static const BlinkStep busy[]    = {{1, 1000}};
  static const BlinkStep failing[] = {{1, 150}, {0, 150}, {1, 150}, {0, 1500}};
  static const BlinkStep ok[]      = {{1, 120}, {0, 9880}};
  switch (state) {
    case STATUS_LED_CONNECTING:
    case STATUS_LED_PORTAL:  steps = fast;    count = 2; return;
    case STATUS_LED_BUSY:    steps = busy;    count = 1; return;
    case STATUS_LED_FAILING: steps = failing; count = 4; return;
    case STATUS_LED_OK:      steps = ok;      count = 2; return;
    default:                 steps = off;     count = 1; return;
  }
}

// Total length of a pattern in milliseconds
inline unsigned long blinkPatternMs(const BlinkStep* steps, int count) {
  unsigned long total = 0;
  for (int i = 0; i < count; i++) total += steps[i].ms;
  return total;
}

// Whether the LED is on at time t (ms) into a repeating pattern. An empty pattern is off.
inline bool blinkPatternOn(const BlinkStep* steps, int count, unsigned long t) {
  unsigned long total = blinkPatternMs(steps, count);
  if (total == 0) return false;
  t %= total;
  for (int i = 0; i < count; i++) {
    if (t < steps[i].ms) return steps[i].on != 0;
    t -= steps[i].ms;
  }
  return false;   // not reached
}

// How many parts of the address to show: those that differ between devices on the network. The parts of
// the network mask that are 255 are the same for every device, so 255.255.255.0 (/24) shows the last part,
// 255.255.0.0 (/16) the last two and 255.0.0.0 (/8) the last three. Always at least one part.
inline int addressPartsToShow(const uint8_t mask[4]) {
  int same = 0;
  while (same < 4 && mask[same] == 255) same++;
  int parts = 4 - same;
  return parts < 1 ? 1 : parts;
}

// Builds the blink pattern for the last `parts` parts of an address, for one showing. Returns the number of
// steps written to out (at most maxSteps).
inline int addressBlinkSteps(const uint8_t ip[4], int parts, int unitMs, BlinkStep* out, int maxSteps) {
  int n = 0;
  auto add = [&](bool on, int ms) {
    if (n < maxSteps) out[n++] = BlinkStep{(uint8_t)(on ? 1 : 0), (uint16_t)ms};
  };
  if (parts < 1) parts = 1;
  if (parts > 4) parts = 4;
  for (int p = 4 - parts; p < 4; p++) {
    char digits[4];
    int len = snprintf(digits, sizeof(digits), "%u", (unsigned)ip[p]);
    for (int d = 0; d < len; d++) {
      int value = digits[d] - '0';
      if (value == 0) { add(true, unitMs * 4); add(false, unitMs); }            // 0 = one long blink
      else for (int k = 0; k < value; k++) { add(true, unitMs); add(false, unitMs); }
      if (d < len - 1) add(false, unitMs * 4);                                   // short pause between digits
    }
    if (p < 3) add(false, unitMs * 10);                                           // long pause between the parts
  }
  add(false, unitMs * 20);                                                         // before it is shown again
  return n;
}

// =====================================================================
//  Reading a day of prices from elprisetjustnu.se
//
//  The answer is a JSON array with one flat object per price period, e.g.
//  [{"SEK_per_kWh":0.6327,"EUR_per_kWh":0.0574,"EXR":11.02,"time_start":"2026-11-22T00:00:00+01:00",
//    "time_end":"2026-11-22T00:15:00+01:00"}, ...]
//  Only SEK_per_kWh, time_start and time_end are used. Tested with whole, cut-off and odd answers in
//  tests/, because a cut-off answer is what made lamps lose their prices in v5.0-v5.1.
// =====================================================================

struct PriceEntry { time_t start; time_t end; float price; };   // end == start when the answer had no end
const int MAX_PRICE_ENTRIES = 120;   // a 25-hour day of 15-minute prices is 100

// Finds "key": inside [p, end) and returns a pointer to the first character after the colon, or nullptr
inline const char* jsonValueOf(const char* p, const char* end, const char* key) {
  size_t klen = strlen(key);
  for (; p + klen + 2 <= end; p++) {
    if (*p != '"' || strncmp(p + 1, key, klen) != 0 || p[klen + 1] != '"') continue;
    const char* q = p + klen + 2;
    while (q < end && (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')) q++;
    if (q < end && *q == ':') {
      q++;
      while (q < end && (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n')) q++;
      return q;
    }
  }
  return nullptr;
}

// Copies a JSON string value starting at p (at its opening quote) into out. Returns false if it isn't one.
inline bool jsonStringAt(const char* p, const char* end, char* out, size_t outSize) {
  if (!p || p >= end || *p != '"' || outSize == 0) return false;
  p++;
  size_t n = 0;
  while (p < end && *p != '"') {
    if (n + 1 < outSize) out[n++] = *p;
    p++;
  }
  out[n] = '\0';
  return p < end;   // the closing quote was found
}

// Reads the price periods in a body of length len into out (at most maxOut). Returns how many were read.
// complete is set when the array ended properly with "]"; a cut-off or broken answer leaves it false, so the
// caller can tell "the server sent nothing" from "only part of it arrived". Periods without a price or a
// start time are skipped.
inline int parsePrices(const char* body, size_t len, PriceEntry* out, int maxOut, bool& complete) {
  complete = false;
  if (!body) return 0;
  const char* p = body;
  const char* end = body + len;
  auto skipSpace = [&]() { while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++; };
  skipSpace();
  if (p >= end || *p != '[') return 0;   // not a JSON array, e.g. an error page
  p++;
  int n = 0;
  skipSpace();
  if (p < end && *p == ']') { complete = true; return 0; }
  while (p < end) {
    skipSpace();
    if (p >= end || *p != '{') return n;
    const char* objEnd = p + 1;
    bool inString = false;
    while (objEnd < end && (inString || *objEnd != '}')) {
      if (*objEnd == '"') inString = !inString;
      else if (!inString && (*objEnd == '{' || *objEnd == '[')) return n;   // the price objects are flat
      objEnd++;
    }
    if (objEnd >= end) return n;   // cut off inside an object

    char ts[40], te[40];
    const char* pv = jsonValueOf(p, objEnd, "SEK_per_kWh");
    char* numEnd = nullptr;
    double price = pv ? strtod(pv, &numEnd) : 0;
    bool hasPrice = pv && numEnd != pv && numEnd <= objEnd;
    time_t a = 0, b = 0;
    bool hasStart = jsonStringAt(jsonValueOf(p, objEnd, "time_start"), objEnd, ts, sizeof(ts)) && parseIso(ts, a);
    if (!(jsonStringAt(jsonValueOf(p, objEnd, "time_end"), objEnd, te, sizeof(te)) && parseIso(te, b))) b = a;
    if (hasPrice && hasStart && n < maxOut) out[n++] = PriceEntry{a, b, (float)price};

    p = objEnd + 1;
    skipSpace();
    if (p < end && *p == ',') { p++; continue; }
    if (p < end && *p == ']') { complete = true; return n; }
    return n;   // cut off, or something else than , or ]
  }
  return n;
}
