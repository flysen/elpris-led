// Board-specific includes and settings. Elpris-LED runs on ESP32 from v5.0; ESP8266 support ended with v4.0.5.
//
// Note: this file is included before FastLED, so it must not define macros with names that
// FastLED uses internally (such as DATA_PIN, which FastLED uses as a template parameter name).
// Constants are therefore constexpr variables, not #defines.
#pragma once
#include <stdint.h>

#if !defined(ESP32)
  #error "Elpris-LED v5 runs on ESP32 only. ESP8266 support ended with v4.0.5."
#endif

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Ticker.h>

#define BOARD_NAME "esp32"                  // part of the update file names; must never change
constexpr uint8_t  DATA_PIN       = 16;     // GPIO16, marked D16 or RX2 on most DevKit boards
constexpr uint8_t  STATUS_LED_PIN = 2;      // the blue LED on most ESP32 DevKit boards
constexpr uint16_t OTA_PORT       = 3232;   // Arduino OTA port on ESP32

inline void setHttpTimeouts(HTTPClient& http) {
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
}
inline void startNtp(const char* tz, const char* server) { configTzTime(tz, server); }

// Largest single block of free memory, shown in the web UI to diagnose fetch problems
inline uint32_t maxFreeBlock() { return ESP.getMaxAllocHeap(); }

// Last three bytes of the MAC address, e.g. "a1b2c3", to tell lamps apart on the network
inline String chipSuffix() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[7];
  snprintf(buf, sizeof(buf), "%02x%02x%02x",
           (unsigned)((mac >> 24) & 0xFF), (unsigned)((mac >> 32) & 0xFF), (unsigned)((mac >> 40) & 0xFF));
  return String(buf);
}

// mDNS is started by the firmware itself, so Arduino OTA must not start it again
inline void startOta() {
  ArduinoOTA.setMdnsEnabled(false);
  ArduinoOTA.begin();
}
