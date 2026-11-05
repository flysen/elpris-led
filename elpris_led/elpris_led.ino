// Elpris-LED v2
// Kräver: ArduinoJson 7.x, FastLED, WiFiManager (tzapu), ESP32 Arduino core
//
// Ändringar jämfört med v1:
//  - Tidszon konfigurerbar (webb + WiFiManager), sparas i flash
//  - Antal LED konfigurerbart (max MAX_LEDS), 15-minuterspriser skalas till slingan
//  - Få API-anrop: dagens priser 1 gång/dygn, morgondagens från 13:30 tills de finns
//  - Exponentiell backoff + visuell felindikering
//  - Cachad data används så länge den täcker aktuell tid ("offline" först när den tar slut)
//  - Validering av inställningar, WiFiManager-fält visar sparade värden
//  - Inga blockerande anrop i webbhanteraren
//  - OTA- och portallösenord sätts i webbgränssnittet och sparas i flash (inga lösenord i koden)

#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <time.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <WebServer.h>

// ---------- HÅRDVARA ----------
#define DATA_PIN     16
#define MAX_LEDS     300       // övre gräns, själva antalet ställs in i webbgränssnittet
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB

// ---------- KONSTANTER ----------
const char* NTP_SERVER = "pool.ntp.org";
const char* DEFAULT_TZ = "CET-1CEST,M3.5.0,M10.5.0/3";   // Sverige

const int SLOT_SEC        = 900;      // 15 minuter
const int SLOTS_PER_DAY   = 96;
const int RING_SIZE       = 512;      // cache för ca 5 dygn av 15-minutersslots

const int TOMORROW_FETCH_MIN = 13 * 60 + 30;  // börja leta efter morgondagens priser 13:30 lokal tid
const int TOMORROW_ERROR_MIN = 16 * 60;       // först efter 16:00 räknas saknade morgondagspriser som fel
const int STATUS_LEDS        = 3;             // antal LED som blinkar magenta vid hämtningsfel

const unsigned long BACKOFF_MIN_MS         = 30UL * 1000UL;
const unsigned long BACKOFF_MAX_MS         = 30UL * 60UL * 1000UL;
const unsigned long NOT_PUBLISHED_RETRY_MS  = 15UL * 60UL * 1000UL;

// ---------- LED-DATA ----------
CRGB leds[MAX_LEDS];
CRGB ledColor[MAX_LEDS];
int  winStartSlot = 0;     // första slot i visningsfönstret (lokal midnatt idag)
int  winSlots     = SLOTS_PER_DAY;   // ett eller två dygn, räknat från verkliga midnatter (DST-säkert)

// ---------- PRISCACHE (ringbuffer, nyckel = absolut 15-minutersslot sedan epoch) ----------
int32_t slotId[RING_SIZE];
float   slotPrice[RING_SIZE];

inline bool  hasSlot(int32_t s)   { return s >= 0 && slotId[s % RING_SIZE] == s; }
inline float getSlot(int32_t s)   { return slotPrice[s % RING_SIZE]; }
inline void  putSlot(int32_t s, float p) { slotId[s % RING_SIZE] = s; slotPrice[s % RING_SIZE] = p; }
void clearCache() { for (int i = 0; i < RING_SIZE; i++) slotId[i] = -1; }

// ---------- INSTÄLLNINGAR ----------
Preferences preferences;
char cfgArea[5]  = "SE3";
char cfgTz[64];
int  cfgNightStart  = 23;
int  cfgNightEnd    = 6;
int  cfgBrightDay   = 40;
int  cfgBrightNight = 5;
int  cfgNumLeds     = 100;
char cfgOtaPw[33]   = "";   // tomt = OTA avstängt
char cfgApPw[33]    = "";   // tomt = öppen konfigportal, annars minst 8 tecken

// WiFiManager-fält (värden fylls i från flash i setup)
WiFiManagerParameter pArea  ("area",   "Elprisomrade (SE1-SE4)",            "", 5);
WiFiManagerParameter pTz    ("tz",     "Tidszon (POSIX TZ-strang)",         "", 63);
WiFiManagerParameter pNstart("nstart", "Nattborjan timme (0-23)",            "", 3);
WiFiManagerParameter pNend  ("nend",   "Nattslut timme (0-23)",              "", 3);
WiFiManagerParameter pBday  ("bday",   "Ljusstyrka dag (1-255)",             "", 4);
WiFiManagerParameter pBnight("bnight", "Ljusstyrka natt (0-255)",            "", 4);
WiFiManagerParameter pLeds  ("leds",   "Antal LED i slingan",                "", 4);
bool shouldSaveConfig = false;
void saveConfigCallback() { shouldSaveConfig = true; }

WebServer server(80);

// ---------- STATUS ----------
bool         needRebuild   = true;
int          lastMday      = -1;
unsigned long nextAttemptMs = 0;
int          failStreak    = 0;
bool         fetchFailing  = false;
unsigned long restartAtMs   = 0;
bool         otaActive     = false;

// =====================================================================
//  Inställningar: ladda / validera / spara
// =====================================================================
void sanitizeSettings() {
  for (int i = 0; cfgArea[i]; i++) cfgArea[i] = toupper(cfgArea[i]);
  bool areaOk = strlen(cfgArea) == 3 && cfgArea[0] == 'S' && cfgArea[1] == 'E' &&
                cfgArea[2] >= '1' && cfgArea[2] <= '4';
  if (!areaOk) strcpy(cfgArea, "SE3");

  cfgNightStart  = constrain(cfgNightStart, 0, 23);
  cfgNightEnd    = constrain(cfgNightEnd, 0, 23);
  cfgBrightDay   = constrain(cfgBrightDay, 1, 255);
  cfgBrightNight = constrain(cfgBrightNight, 0, 255);
  cfgNumLeds     = constrain(cfgNumLeds, 1, MAX_LEDS);

  if (strlen(cfgTz) < 3) strlcpy(cfgTz, DEFAULT_TZ, sizeof(cfgTz));
  if (strlen(cfgApPw) > 0 && strlen(cfgApPw) < 8) cfgApPw[0] = '\0';   // WPA2 kräver minst 8 tecken
}

void loadSettings() {
  String a = preferences.getString("area", "SE3");
  a.toCharArray(cfgArea, sizeof(cfgArea));
  String z = preferences.getString("tz", DEFAULT_TZ);
  z.toCharArray(cfgTz, sizeof(cfgTz));
  cfgNightStart  = preferences.getInt("nstart", 23);
  cfgNightEnd    = preferences.getInt("nend", 6);
  cfgBrightDay   = preferences.getInt("bday", 40);
  cfgBrightNight = preferences.getInt("bnight", 5);
  cfgNumLeds     = preferences.getInt("leds", 100);
  preferences.getString("otapw", "").toCharArray(cfgOtaPw, sizeof(cfgOtaPw));
  preferences.getString("appw", "").toCharArray(cfgApPw, sizeof(cfgApPw));
  sanitizeSettings();
}

void saveSettings() {
  preferences.putString("area", cfgArea);
  preferences.putString("tz", cfgTz);
  preferences.putInt("nstart", cfgNightStart);
  preferences.putInt("nend", cfgNightEnd);
  preferences.putInt("bday", cfgBrightDay);
  preferences.putInt("bnight", cfgBrightNight);
  preferences.putInt("leds", cfgNumLeds);
  preferences.putString("otapw", cfgOtaPw);
  preferences.putString("appw", cfgApPw);
}

void applyTimezone() {
  setenv("TZ", cfgTz, 1);
  tzset();
}

// =====================================================================
//  Tidshjälp
// =====================================================================
long daysFromCivil(int y, int m, int d) {
  y -= (m <= 2);
  long era = (y >= 0 ? y : y - 399) / 400;
  int yoe = y - era * 400;
  int mp  = (m > 2) ? m - 3 : m + 9;
  int doy = (153 * mp + 2) / 5 + d - 1;
  long doe = yoe * 365L + yoe / 4 - yoe / 100 + doy;
  return era * 146097L + doe - 719468L;
}

bool parseIso(const char* s, time_t& out) {
  int Y, M, D, h, m, sec, oh = 0, om = 0;
  char sign = 0;
  int n = sscanf(s, "%d-%d-%dT%d:%d:%d%c%d:%d", &Y, &M, &D, &h, &m, &sec, &sign, &oh, &om);
  if (n < 6) return false;
  long t = daysFromCivil(Y, M, D) * 86400L + h * 3600L + m * 60L + sec;
  if (n >= 9 && (sign == '+' || sign == '-')) {
    long off = oh * 3600L + om * 60L;
    t -= (sign == '+') ? off : -off;
  }
  out = (time_t)t;
  return true;
}

int32_t localMidnightSlot(time_t now, int dayOffset) {
  struct tm t;
  localtime_r(&now, &t);
  t.tm_mday += dayOffset;
  t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
  t.tm_isdst = -1;
  return (int32_t)(mktime(&t) / SLOT_SEC);
}

bool dayCovered(int32_t startSlot) {
  return hasSlot(startSlot + 2) && hasSlot(startSlot + 50) && hasSlot(startSlot + 90);
}

String priceUrl(time_t now, int dayOffset) {
  struct tm t;
  localtime_r(&now, &t);
  t.tm_mday += dayOffset;
  t.tm_isdst = -1;
  mktime(&t);
  char buf[16];
  strftime(buf, sizeof(buf), "%Y/%m-%d", &t);
  return String("https://www.elprisetjustnu.se/api/v1/prices/") + buf + "_" + cfgArea + ".json";
}

// =====================================================================
//  Hämtning av priser
// =====================================================================
bool fetchDay(const String& url, int& code) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.useHTTP10(true);
  if (!http.begin(client, url)) { code = -1; return false; }

  code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return false; }

  JsonDocument filter;
  filter[0]["SEK_per_kWh"] = true;
  filter[0]["time_start"]  = true;
  filter[0]["time_end"]    = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { code = -2; return false; }

  int stored = 0;
  for (JsonObject o : doc.as<JsonArray>()) {
    const char* ts = o["time_start"];
    const char* te = o["time_end"];
    if (!ts) continue;
    time_t a, b;
    if (!parseIso(ts, a)) continue;
    if (!te || !parseIso(te, b) || b <= a) b = a + 3600;
    float p = o["SEK_per_kWh"] | 0.0f;
    int32_t s0 = (int32_t)(a / SLOT_SEC);
    int32_t s1 = (int32_t)((b + SLOT_SEC - 1) / SLOT_SEC);
    for (int32_t s = s0; s < s1; s++) { putSlot(s, p); stored++; }
  }
  if (stored == 0) { code = -3; return false; }
  return true;
}

void serviceFetch(const struct tm& t, time_t now) {
  if ((long)(millis() - nextAttemptMs) < 0) return;

  int32_t todaySlot    = localMidnightSlot(now, 0);
  int32_t tomorrowSlot = localMidnightSlot(now, 1);
  int minutes = t.tm_hour * 60 + t.tm_min;

  int dayOffset;
  if (!dayCovered(todaySlot)) dayOffset = 0;
  else if (minutes >= TOMORROW_FETCH_MIN && !dayCovered(tomorrowSlot)) dayOffset = 1;
  else return;

  int code = 0;
  bool ok = fetchDay(priceUrl(now, dayOffset), code);
  unsigned long ms = millis();

  if (ok) {
    failStreak = 0;
    fetchFailing = false;
    needRebuild = true;
    int32_t startSlot = dayOffset ? tomorrowSlot : todaySlot;
    nextAttemptMs = ms + (dayCovered(startSlot) ? 1000UL : NOT_PUBLISHED_RETRY_MS);
    Serial.printf("Priser hämtade (dag +%d)\n", dayOffset);
  } else if (dayOffset == 1 && code == 404) {
    nextAttemptMs = ms + NOT_PUBLISHED_RETRY_MS;
    fetchFailing = (minutes >= TOMORROW_ERROR_MIN);
    Serial.println("Morgondagens priser inte publicerade än");
  } else {
    failStreak++;
    int shift = (failStreak - 1 > 6) ? 6 : failStreak - 1;
    unsigned long d = BACKOFF_MIN_MS << shift;
    if (d > BACKOFF_MAX_MS) d = BACKOFF_MAX_MS;
    nextAttemptMs = ms + d;
    fetchFailing = (dayOffset == 0) || (minutes >= TOMORROW_ERROR_MIN);
    Serial.printf("Hämtning misslyckades (kod %d), nytt försök om %lu s\n", code, d / 1000);
  }
}

// =====================================================================
//  Bygg färgerna för slingan
// =====================================================================
inline void ledRange(int i, int& s0, int& s1) {
  s0 = (int)((long)i * winSlots / cfgNumLeds);
  s1 = (int)((long)(i + 1) * winSlots / cfgNumLeds);
  if (s1 <= s0) s1 = s0 + 1;
}

void rebuildTimeline(time_t now) {
  // Fönstret räknas från verkliga midnatter så att 23- och 25-timmarsdygn (sommartid) blir rätt
  winStartSlot = localMidnightSlot(now, 0);
  int32_t tomorrowStart = localMidnightSlot(now, 1);
  int32_t dayAfterStart = localMidnightSlot(now, 2);
  bool tomorrow = dayCovered(tomorrowStart);
  winSlots = (tomorrow ? dayAfterStart : tomorrowStart) - winStartSlot;

  float mn = 1e9f, mx = -1e9f;
  for (int s = 0; s < winSlots; s++) {
    if (!hasSlot(winStartSlot + s)) continue;
    float p = getSlot(winStartSlot + s);
    if (p < mn) mn = p;
    if (p > mx) mx = p;
  }

  CRGB prev = CRGB::Black;
  for (int i = 0; i < cfgNumLeds; i++) {
    int s0, s1;
    ledRange(i, s0, s1);
    float sum = 0; int cnt = 0;
    for (int s = s0; s < s1; s++) {
      if (hasSlot(winStartSlot + s)) { sum += getSlot(winStartSlot + s); cnt++; }
    }
    if (cnt == 0) {
      ledColor[i] = prev;
    } else {
      float p = sum / cnt;
      float ratio = (mx > mn) ? (p - mn) / (mx - mn) : 0.5f;
      uint8_t hue = (uint8_t)((1.0f - ratio) * 96);
      ledColor[i] = CHSV(hue, 255, 255);
    }
    prev = ledColor[i];
  }
}

// =====================================================================
//  Webbgränssnitt
// =====================================================================
String htmlEscape(const String& s) {
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '&':  o += "&amp;";  break;
      case '<':  o += "&lt;";   break;
      case '>':  o += "&gt;";   break;
      case '"':  o += "&quot;"; break;
      case '\'': o += "&#39;";  break;
      default:   o += c;
    }
  }
  return o;
}

void handleRoot() {
  time_t now = time(nullptr);
  int32_t ns = (int32_t)(now / SLOT_SEC);
  String priceTxt = (now > 1700000000 && hasSlot(ns)) ? String(getSlot(ns), 2) + " SEK/kWh" : String("saknas");
  String status = fetchFailing ? "Kan inte hämta priser (försöker igen automatiskt)" : "OK";
  String otaTxt = otaActive ? "Aktiv" : "Av (sätt ett OTA-lösenord nedan)";

  String html;
  html.reserve(5500);
  html += "<html lang='sv'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif;background:#121212;color:#e0e0e0;padding:20px;max-width:400px;margin:auto;}";
  html += "input,select{width:100%;padding:10px;margin:8px 0;background:#222;color:#fff;border:1px solid #444;border-radius:4px;box-sizing:border-box;}";
  html += "button{width:100%;padding:12px;background:#4CAF50;color:white;border:none;border-radius:4px;font-size:16px;font-weight:bold;cursor:pointer;}";
  html += ".card{background:#1e1e1e;padding:15px;border-radius:8px;margin-bottom:15px;border:1px solid #333;}small{color:#999;}</style></head><body>";

  html += "<h2>Elpris LED</h2><div class='card'><h3>Live Status</h3>";
  html += "<p><strong>Pris just nu:</strong> " + priceTxt + "<br><small>Spotpris exkl. moms, nät och skatt</small></p>";
  html += "<p><strong>Område:</strong> " + String(cfgArea) + "</p>";
  html += "<p><strong>Status:</strong> " + status + "</p>";
  html += "<p><strong>OTA:</strong> " + otaTxt + "</p></div>";

  html += "<div class='card'><h3>Inställningar</h3><form action='/save' method='POST'>";
  html += "<label>Elprisområde:</label><select name='area'>";
  const char* areas[] = {"SE1", "SE2", "SE3", "SE4"};
  for (int i = 0; i < 4; i++) {
    html += "<option value='" + String(areas[i]) + "'" + (String(cfgArea) == areas[i] ? " selected" : "") + ">" + areas[i] + "</option>";
  }
  html += "</select>";

  html += "<label>Antal LED (ändring startar om enheten):</label><input type='number' name='leds' min='1' max='" + String(MAX_LEDS) + "' value='" + String(cfgNumLeds) + "'>";

  html += "<label>Tidszon (förval):</label><select onchange=\"if(this.value)document.getElementById('tz').value=this.value\">";
  html += "<option value=''>Välj...</option>";
  html += "<option value='CET-1CEST,M3.5.0,M10.5.0/3'>Sverige / Mellaneuropa</option>";
  html += "<option value='EET-2EEST,M3.5.0/3,M10.5.0/4'>Finland / Baltikum</option>";
  html += "<option value='GMT0BST,M3.5.0/1,M10.5.0'>Storbritannien</option>";
  html += "<option value='UTC0'>UTC</option></select>";
  html += "<label>Tidszon (POSIX TZ-sträng):</label><input type='text' id='tz' name='tz' maxlength='63' value='" + htmlEscape(String(cfgTz)) + "'>";

  html += "<label>Nattläge Start (timme 0-23):</label><input type='number' name='nstart' min='0' max='23' value='" + String(cfgNightStart) + "'>";
  html += "<label>Nattläge Slut (timme 0-23):</label><input type='number' name='nend' min='0' max='23' value='" + String(cfgNightEnd) + "'>";
  html += "<label>Ljusstyrka Dag (1-255):</label><input type='number' name='bday' min='1' max='255' value='" + String(cfgBrightDay) + "'>";
  html += "<label>Ljusstyrka Natt (0-255):</label><input type='number' name='bnight' min='0' max='255' value='" + String(cfgBrightNight) + "'>";

  html += "<label>OTA-lösenord (tomt = oförändrat, ändring startar om):</label>";
  html += "<input type='password' name='otapw' maxlength='32' autocomplete='new-password' placeholder='" + String(strlen(cfgOtaPw) ? "(satt)" : "(inte satt)") + "'>";
  html += "<label>Lösenord för konfigportalen, minst 8 tecken (tomt = oförändrat):</label>";
  html += "<input type='password' name='appw' minlength='8' maxlength='32' autocomplete='new-password' placeholder='" + String(strlen(cfgApPw) ? "(satt)" : "(öppen portal)") + "'>";

  html += "<button type='submit'>Spara ändringar</button></form></div></body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  char oldArea[5], oldTz[64];
  strlcpy(oldArea, cfgArea, sizeof(oldArea));
  strlcpy(oldTz, cfgTz, sizeof(oldTz));
  int oldLeds = cfgNumLeds;
  bool otaPwChanged = false;

  if (server.hasArg("area"))   server.arg("area").toCharArray(cfgArea, sizeof(cfgArea));
  if (server.hasArg("tz"))     server.arg("tz").toCharArray(cfgTz, sizeof(cfgTz));
  if (server.hasArg("nstart")) cfgNightStart  = server.arg("nstart").toInt();
  if (server.hasArg("nend"))   cfgNightEnd    = server.arg("nend").toInt();
  if (server.hasArg("bday"))   cfgBrightDay   = server.arg("bday").toInt();
  if (server.hasArg("bnight")) cfgBrightNight = server.arg("bnight").toInt();
  if (server.hasArg("leds"))   cfgNumLeds     = server.arg("leds").toInt();

  if (server.hasArg("otapw") && server.arg("otapw").length() > 0) {
    String pw = server.arg("otapw");
    if (pw != cfgOtaPw) { pw.toCharArray(cfgOtaPw, sizeof(cfgOtaPw)); otaPwChanged = true; }
  }
  if (server.hasArg("appw") && server.arg("appw").length() >= 8) {
    server.arg("appw").toCharArray(cfgApPw, sizeof(cfgApPw));   // gäller nästa gång portalen öppnas
  }

  sanitizeSettings();
  saveSettings();

  if (strcmp(oldTz, cfgTz) != 0) { applyTimezone(); needRebuild = true; }
  if (strcmp(oldArea, cfgArea) != 0) {
    clearCache();
    nextAttemptMs = 0;
    failStreak = 0;
    fetchFailing = false;
    needRebuild = true;
  }
  if (oldLeds != cfgNumLeds || otaPwChanged) restartAtMs = millis() + 1500;

  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

// =====================================================================
//  Setup / loop
// =====================================================================
void setup() {
  Serial.begin(115200);
  clearCache();

  preferences.begin("elpris-config", false);
  loadSettings();

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, cfgNumLeds);
  FastLED.setBrightness(cfgBrightDay);
  fill_solid(leds, cfgNumLeds, CRGB::Blue);
  FastLED.show();

  pArea.setValue(cfgArea, 5);
  pTz.setValue(cfgTz, 63);
  pNstart.setValue(String(cfgNightStart).c_str(), 3);
  pNend.setValue(String(cfgNightEnd).c_str(), 3);
  pBday.setValue(String(cfgBrightDay).c_str(), 4);
  pBnight.setValue(String(cfgBrightNight).c_str(), 4);
  pLeds.setValue(String(cfgNumLeds).c_str(), 4);

  WiFiManager wm;
  wm.addParameter(&pArea);
  wm.addParameter(&pTz);
  wm.addParameter(&pNstart);
  wm.addParameter(&pNend);
  wm.addParameter(&pBday);
  wm.addParameter(&pBnight);
  wm.addParameter(&pLeds);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setConnectTimeout(30);         // försök ansluta till sparat nät i 30 s
  wm.setConfigPortalTimeout(180);   // stäng portalen efter 3 min och starta om, så att wifi provas igen

  const char* apPw = strlen(cfgApPw) ? cfgApPw : nullptr;   // inget lösenord satt = öppen portal
  if (!wm.autoConnect("Elpris-LED-Config", apPw)) {
    Serial.println("Kunde inte ansluta, startar om...");
    delay(3000);
    ESP.restart();
  }

  if (shouldSaveConfig) {
    int oldLeds = cfgNumLeds;
    strlcpy(cfgArea, pArea.getValue(), sizeof(cfgArea));
    strlcpy(cfgTz, pTz.getValue(), sizeof(cfgTz));
    cfgNightStart  = String(pNstart.getValue()).toInt();
    cfgNightEnd    = String(pNend.getValue()).toInt();
    cfgBrightDay   = String(pBday.getValue()).toInt();
    cfgBrightNight = String(pBnight.getValue()).toInt();
    cfgNumLeds     = String(pLeds.getValue()).toInt();
    sanitizeSettings();
    saveSettings();
    if (oldLeds != cfgNumLeds) { delay(500); ESP.restart(); }
  }

  Serial.print("IP-adress: ");
  Serial.println(WiFi.localIP());

  // OTA startas bara när ett lösenord är satt i webbgränssnittet
  if (strlen(cfgOtaPw)) {
    ArduinoOTA.setHostname("ElprisLED");
    ArduinoOTA.setPassword(cfgOtaPw);
    ArduinoOTA.begin();
    otaActive = true;
  } else {
    Serial.println("OTA av: sätt ett OTA-lösenord i webbgränssnittet");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  configTzTime(cfgTz, NTP_SERVER);
}

void loop() {
  if (otaActive) ArduinoOTA.handle();
  server.handleClient();
  if (restartAtMs && (long)(millis() - restartAtMs) >= 0) ESP.restart();

  time_t now = time(nullptr);

  if (now < 1700000000) {
    uint8_t b = beatsin8(20, 20, 160);
    fill_solid(leds, cfgNumLeds, CRGB(0, 0, b));
    FastLED.show();
    delay(20);
    return;
  }

  struct tm t;
  localtime_r(&now, &t);

  if (t.tm_mday != lastMday) { lastMday = t.tm_mday; needRebuild = true; }

  serviceFetch(t, now);
  if (needRebuild) { rebuildTimeline(now); needRebuild = false; }

  bool isNight;
  if (cfgNightStart > cfgNightEnd) isNight = (t.tm_hour >= cfgNightStart || t.tm_hour < cfgNightEnd);
  else                             isNight = (t.tm_hour >= cfgNightStart && t.tm_hour < cfgNightEnd);
  FastLED.setBrightness(isNight ? cfgBrightNight : cfgBrightDay);

  int32_t nowSlot = (int32_t)(now / SLOT_SEC);

  if (!hasSlot(nowSlot)) {
    uint8_t b = beatsin8(20, 20, 160);
    if (failStreak > 0) fill_solid(leds, cfgNumLeds, CRGB(b, 0, b));
    else                fill_solid(leds, cfgNumLeds, CRGB(0, 0, b));
    FastLED.show();
    delay(20);
    return;
  }

  int cur = nowSlot - winStartSlot;
  for (int i = 0; i < cfgNumLeds; i++) {
    int s0, s1;
    ledRange(i, s0, s1);
    CRGB c = ledColor[i];
    if (cur >= s0 && cur < s1) c.nscale8(beatsin8(30, 30, 255));
    leds[i] = c;
  }

  if (fetchFailing) {
    CRGB c = ((millis() / 500) % 2) ? CRGB::Magenta : CRGB::Black;
    for (int i = 0; i < STATUS_LEDS && i < cfgNumLeds; i++) leds[i] = c;
  }

  FastLED.show();
  delay(20);
}
