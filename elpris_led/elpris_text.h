// The texts in the lamp's web page, in English and Swedish. No Arduino dependencies, so it is tested on a
// regular computer (see tests/).
//
// Every text is one line below: an id, the English text and the Swedish text. Adding a text means adding a
// line, so a translation can't go missing or end up in the wrong place. English is written first; the
// Swedish is a translation of it. Texts that end or start with a space are pieces of a sentence that the
// page puts together with numbers or names.
//
// The config portal (the wifi Elpris-LED-Config) stays in English: it opens before the lamp knows which
// language to use, and most of it comes from the WiFiManager library.
#pragma once
#include <stddef.h>

enum Lang { LANG_EN = 0, LANG_SV = 1, LANG_COUNT = 2 };

// The language setting stored in the lamp (key "lang")
const int LANG_SETTING_AUTO = 0;   // the browser's language: Swedish if it asks for it, otherwise English
const int LANG_SETTING_EN   = 1;
const int LANG_SETTING_SV   = 2;

#define ELPRIS_TEXTS(X) \
  /* Live status */ \
  X(TXT_LIVE_STATUS, "Live status", "Just nu") \
  X(TXT_CURRENT_PRICE, "Current price", "Pris just nu") \
  X(TXT_SPOT_NOTE, "Spot price excl. VAT, grid fees and tax", "Spotpris exkl. moms, nätavgift och skatt") \
  X(TXT_UNAVAILABLE, "unavailable", "saknas") \
  X(TXT_NORMAL_PRICE, "Normal price", "Normalpris") \
  X(TXT_AVERAGE_OF, "Average of ", "Snitt av ") \
  X(TXT_PAST_DAY, " past day, ", " föregående dag, ") \
  X(TXT_PAST_DAYS, " past days, ", " föregående dagar, ") \
  X(TXT_TODAY, "today", "idag") \
  X(TXT_AND_TOMORROW, " and tomorrow", " och imorgon") \
  X(TXT_YELLOW_WITHIN, ". Yellow within ±", ". Gult inom ±") \
  X(TXT_NOT_KNOWN_YET, "not known yet", "inte känt än") \
  X(TXT_TOMORROW_VS_TODAY, "Tomorrow vs today", "Imorgon jämfört med idag") \
  X(TXT_NOT_PUBLISHED, "not published yet", "inte publicerat än") \
  X(TXT_AREA, "Area", "Elområde") \
  X(TXT_STATUS, "Status", "Status") \
  X(TXT_STATUS_OK, "OK", "OK") \
  X(TXT_STATUS_FAILING, "Cannot fetch prices (retrying automatically)", "Kan inte hämta priser (försöker igen automatiskt)") \
  X(TXT_LAST_ERROR, "Last error: ", "Senaste fel: ") \
  X(TXT_OTA_ACTIVE, "Active", "På") \
  X(TXT_OTA_OFF, "Off (set an OTA password below)", "Av (sätt ett OTA-lösenord nedan)") \
  X(TXT_FIRMWARE, "Firmware", "Firmware") \
  X(TXT_LATEST_RELEASE, ", latest release: ", ", senaste version: ") \
  X(TXT_LATEST_TEST, ", latest version incl. test versions: ", ", senaste version inkl. testversioner: ") \
  X(TXT_TEST_CHANNEL_NOTE, "Test channel: release candidates are installed too.", "Testkanal: testversioner installeras också.") \
  X(TXT_ADDRESS, "Address", "Adress") \
  X(TXT_OR, " or ", " eller ") \
  X(TXT_ROUTER_NAME, "Name in the router: ", "Namn i routern: ") \
  X(TXT_FREE_MEMORY, "Free memory: ", "Fritt minne: ") \
  X(TXT_BYTES_LARGEST, " bytes, largest block ", " byte, största block ") \
  X(TXT_BYTES, " bytes", " byte") \
  /* Updates */ \
  X(TXT_UPDATES, "Updates", "Uppdateringar") \
  X(TXT_CHECK_NOW, "Check for updates now", "Sök efter uppdateringar nu") \
  X(TXT_UPDATES_HELP, "Installs a newer release if there is one and restarts into it, which takes about a minute.", \
    "Installerar en nyare version om det finns en och startar om med den. Det tar ungefär en minut.") \
  X(TXT_UPD_CHECKING, "Checking for updates...", "Söker efter uppdateringar...") \
  X(TXT_UPD_UP_TO_DATE, "Already up to date", "Senaste versionen är redan installerad") \
  X(TXT_UPD_CHECK_FAILED, "Could not check for updates", "Kunde inte söka efter uppdateringar") \
  X(TXT_UPD_AVAILABLE, " is available", " finns tillgänglig") \
  X(TXT_UPD_INSTALLING, "Installing ", "Installerar ") \
  X(TXT_UPD_INSTALLING_END, "... The lamp restarts when it is done, in about a minute.", \
    "... Lampan startar om när den är klar, om ungefär en minut.") \
  X(TXT_UPD_FAILED, "Update to ", "Uppdateringen till ") \
  X(TXT_UPD_FAILED_END, " failed: ", " misslyckades: ") \
  X(TXT_UPD_NOT_INSTALLED, "No update installed", "Ingen uppdatering installerades") \
  /* Settings */ \
  X(TXT_SETTINGS, "Settings", "Inställningar") \
  X(TXT_PRICE_AREA, "Price area:", "Elområde:") \
  X(TXT_NUM_LEDS, "Number of LEDs (changing it restarts the device):", "Antal lysdioder (en ändring startar om lampan):") \
  X(TXT_COLORS, "Colors", "Färger") \
  X(TXT_MODE_NORMAL, "Compared with a normal price", "Jämfört med ett normalpris") \
  X(TXT_MODE_RANGE, "Cheapest to most expensive", "Billigast till dyrast") \
  X(TXT_COLORS_HELP, \
    "<b>Compared with a normal price:</b> today from midnight to midnight, green when cheaper than usual, yellow when " \
    "it doesn't matter, red when more expensive. The last three LEDs are one off LED and two that compare tomorrow " \
    "08-17 and 17-24 with the same hours today (off until tomorrow's prices are published). <b>Cheapest to most " \
    "expensive:</b> today, and tomorrow once published, from green to red within that period.", \
    "<b>Jämfört med ett normalpris:</b> idag från midnatt till midnatt, grönt när det är billigare än vanligt, gult " \
    "när det inte spelar någon roll, rött när det är dyrare. De tre sista lysdioderna är en släckt och två som " \
    "jämför imorgon 08-17 och 17-24 med samma timmar idag (släckta tills morgondagens priser är publicerade). " \
    "<b>Billigast till dyrast:</b> idag, och imorgon när priserna är publicerade, från grönt till rött inom den perioden.") \
  X(TXT_HIST_DAYS, "Normal price: past days (0-7):", "Normalpris: föregående dagar (0-7):") \
  X(TXT_NEUTRAL, "Doesn't matter within (± öre):", "Spelar ingen roll inom (± öre):") \
  X(TXT_STRONG, "Very cheap or expensive from (± öre):", "Mycket billigt eller dyrt från (± öre):") \
  X(TXT_LEVELS, "Levels:", "Nivåer:") \
  X(TXT_LEVELS_5, "5: green, yellow-green, yellow, orange, red", "5: grönt, gulgrönt, gult, orange, rött") \
  X(TXT_LEVELS_3, "3: green, yellow, red", "3: grönt, gult, rött") \
  X(TXT_NORM_TOMORROW, "Count tomorrow in the normal price", "Räkna med imorgon i normalpriset") \
  X(TXT_NORM_TOMORROW_HELP, \
    "Today's colors may then change when tomorrow's prices arrive after lunch: an expensive tomorrow makes today greener.", \
    "Dagens färger kan då ändras när morgondagens priser kommer efter lunch: en dyr morgondag gör idag grönare.") \
  X(TXT_NORMAL_NOTE, \
    "These settings apply to <i>Compared with a normal price</i>. Prices are spot prices excl. VAT, in öre.", \
    "De här inställningarna gäller <i>Jämfört med ett normalpris</i>. Priserna är spotpriser exkl. moms, i öre.") \
  X(TXT_STRIP, "Strip", "Slingan") \
  X(TXT_REVERSE, "Reverse direction", "Omvänd riktning") \
  X(TXT_REVERSE_HELP, \
    "Normally midnight is at the end where the strip's cable is connected. Tick this if time runs the wrong way, e.g. right to left.", \
    "Normalt är midnatt i den ände där slingans kabel sitter. Kryssa i om tiden går åt fel håll, till exempel från höger till vänster.") \
  X(TXT_PULSE, "Pulse the current 15 minutes towards white", "Låt den aktuella kvarten pulsera mot vitt") \
  X(TXT_PULSE_HELP, \
    "Makes \"now\" stand out against any price color. Untick to pulse in the price color instead.", \
    "Gör att \"nu\" syns mot alla prisfärger. Kryssa ur för att pulsera i prisfärgen i stället.") \
  X(TXT_PAST_PCT, "Brightness of passed time (0-100 %):", "Ljusstyrka för passerad tid (0-100 %):") \
  X(TXT_PAST_HELP, \
    "Time that has passed is shown at this share of the normal brightness, so the hours that are left stand out. " \
    "100 = not dimmed, 0 = off. The eye sees brightness logarithmically, so 10 % looks clearly dimmed but not dark.", \
    "Tid som har passerat visas med den här andelen av den vanliga ljusstyrkan, så att timmarna som är kvar syns " \
    "tydligare. 100 = inte dämpat, 0 = släckt. Ögat uppfattar ljusstyrka logaritmiskt, så 10 % ser tydligt dämpat ut men inte mörkt.") \
  X(TXT_TIME_ZONE, "Time zone:", "Tidszon:") \
  X(TXT_TZ_CUSTOM, "Custom (", "Egen (") \
  X(TXT_TZ_HELP, \
    "Where the lamp is, so the strip starts at local midnight. Daylight saving time is handled automatically.", \
    "Var lampan finns, så att slingan börjar vid lokal midnatt. Sommartid hanteras automatiskt.") \
  X(TXT_NIGHT_MODE, "Use night mode", "Använd nattläge") \
  X(TXT_NIGHT_MODE_HELP, \
    "Uses the night brightness between the hours below. Untick to use the day brightness around the clock, " \
    "for example in a laundry room or another place where nobody sleeps.", \
    "Använder nattljusstyrkan mellan timmarna nedan. Kryssa ur för att använda dagljusstyrkan dygnet runt, " \
    "till exempel i en tvättstuga eller någon annanstans där ingen sover.") \
  X(TXT_NIGHT_START, "Night mode start (hour 0-23):", "Nattläge börjar (timme 0-23):") \
  X(TXT_NIGHT_END, "Night mode end (hour 0-23):", "Nattläge slutar (timme 0-23):") \
  X(TXT_BRIGHT_DAY, "Day brightness (1-255):", "Ljusstyrka dag (1-255):") \
  X(TXT_BRIGHT_NIGHT, "Night brightness (0-255):", "Ljusstyrka natt (0-255):") \
  X(TXT_DEVICE, "Device", "Enhet") \
  X(TXT_LANGUAGE, "Language / Språk:", "Language / Språk:") \
  X(TXT_LANG_AUTO, "Automatic (the browser's language)", "Automatiskt (webbläsarens språk)") \
  X(TXT_AUTO_UPDATE, "Install new releases automatically", "Installera nya versioner automatiskt") \
  X(TXT_BOARD_LED, "Show status on the board's blue LED", "Visa status med kortets blå lysdiod") \
  X(TXT_BOARD_LED_HELP, \
    "A short blink every 10 seconds when everything works, on while fetching prices or updating, double blinks " \
    "when fetching fails, and the lamp's address as counted blinks after it starts. Setting up wifi always " \
    "blinks fast. Untick to keep it dark.", \
    "En kort blinkning var tionde sekund när allt fungerar, tänd medan priser hämtas eller lampan uppdateras, " \
    "dubbelblinkningar när hämtningen misslyckas och lampans adress som räknade blinkningar efter start. " \
    "När wifi ställs in blinkar den alltid snabbt. Kryssa ur för att hålla den släckt.") \
  X(TXT_TEST_CHANNEL, "Also install test versions (release candidates)", "Installera även testversioner (release candidates)") \
  X(TXT_TEST_CHANNEL_HELP, \
    "For trying new features before they are released, wirelessly. Test versions may have bugs. " \
    "Untick to go back: the lamp then moves on with the next stable release.", \
    "För att prova nya funktioner trådlöst innan de släpps. Testversioner kan ha buggar. " \
    "Kryssa ur för att gå tillbaka: lampan fortsätter då med nästa stabila version.") \
  X(TXT_OTA_PASSWORD, "OTA password (empty = unchanged, changing it restarts the device):", \
    "OTA-lösenord (tomt = oförändrat, en ändring startar om lampan):") \
  X(TXT_PORTAL_PASSWORD, "Config portal password, at least 8 characters (empty = unchanged):", \
    "Lösenord för konfigportalen, minst 8 tecken (tomt = oförändrat):") \
  X(TXT_PW_SET, "(set)", "(satt)") \
  X(TXT_PW_NOT_SET, "(not set)", "(inte satt)") \
  X(TXT_PW_OPEN_PORTAL, "(open portal)", "(öppen portal)") \
  X(TXT_SAVE, "Save changes", "Spara ändringar") \
  /* Footer. The price credit is in Swedish in both languages, as Elpriset just nu.se asks. */ \
  X(TXT_SPOT_FOOTER, "Spot prices excl. VAT, grid fees and tax.", "Spotpriser exkl. moms, nätavgift och skatt.") \
  X(TXT_EMULATOR_BEFORE, "Try the settings without a lamp in the ", "Prova inställningarna utan lampa i ") \
  X(TXT_EMULATOR_LINK, "emulator", "emulatorn") \
  X(TXT_HTML_LANG, "en", "sv")

enum TextId {
#define ELPRIS_TEXT_ID(id, en, sv) id,
  ELPRIS_TEXTS(ELPRIS_TEXT_ID)
#undef ELPRIS_TEXT_ID
  TXT_COUNT
};

// The text in the given language. An unknown language or id gives English or an empty string, never null.
inline const char* tr(Lang lang, int id) {
  static const char* const table[TXT_COUNT][LANG_COUNT] = {
#define ELPRIS_TEXT_ROW(id, en, sv) {en, sv},
    ELPRIS_TEXTS(ELPRIS_TEXT_ROW)
#undef ELPRIS_TEXT_ROW
  };
  if (id < 0 || id >= TXT_COUNT) return "";
  if (lang < 0 || lang >= LANG_COUNT) lang = LANG_EN;
  return table[id][lang];
}

// The language of a page. A setting of English or Swedish always wins. Automatic looks at the browser's
// Accept-Language header, e.g. "sv-SE,sv;q=0.9,en-US;q=0.8,en;q=0.7": the first language it accepts
// (q above 0) that the lamp has decides. Anything else, or no header, gives English.
inline Lang chooseLanguage(int setting, const char* acceptLanguage) {
  if (setting == LANG_SETTING_EN) return LANG_EN;
  if (setting == LANG_SETTING_SV) return LANG_SV;
  if (!acceptLanguage) return LANG_EN;
  const char* p = acceptLanguage;
  while (*p) {
    while (*p == ' ' || *p == ',') p++;
    if (!*p) break;
    // The primary language, e.g. "sv" in "sv-SE"
    char primary[9];
    size_t n = 0;
    while (*p && *p != '-' && *p != ';' && *p != ',' && *p != ' ') {
      char c = *p++;
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      if (n < sizeof(primary) - 1) primary[n++] = c;
    }
    primary[n] = '\0';
    // Skip the rest of the tag, and read q if it is there (q=0 means "not this language")
    bool refused = false;
    while (*p && *p != ',') {
      if (p[0] == ';' && (p[1] == 'q' || p[1] == 'Q') && p[2] == '=') {
        const char* q = p + 3;
        refused = (*q == '0');
        if (refused) {
          q++;
          if (*q == '.') {
            q++;
            while (*q == '0') q++;
            refused = !(*q >= '1' && *q <= '9');
          }
        }
      }
      p++;
    }
    if (refused) continue;
    if (primary[0] == 's' && primary[1] == 'v' && primary[2] == '\0') return LANG_SV;
    if (primary[0] == 'e' && primary[1] == 'n' && primary[2] == '\0') return LANG_EN;
  }
  return LANG_EN;
}
