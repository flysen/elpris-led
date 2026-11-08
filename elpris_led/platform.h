// Board-specific includes and the few differences between ESP32 and ESP8266.
// Everything else in the firmware is the same for both boards.
#pragma once

#if defined(ESP32)

  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  #include <HTTPClient.h>
  #include <HTTPUpdate.h>
  #include <WebServer.h>
  #include <Preferences.h>

  #define BOARD_NAME "esp32"
  #define DATA_PIN   16          // GPIO16, marked D16 or RX2 on most DevKit boards

  inline void setHttpTimeouts(HTTPClient& http) {
    http.setConnectTimeout(5000);
    http.setTimeout(5000);
  }
  inline void startNtp(const char* tz, const char* server) { configTzTime(tz, server); }

#elif defined(ESP8266)

  #define FASTLED_ESP8266_RAW_PIN_ORDER   // DATA_PIN is a GPIO number, like on ESP32
  #include <ESP8266WiFi.h>
  #include <WiFiClientSecure.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266httpUpdate.h>
  #include <ESP8266WebServer.h>
  #include <LittleFS.h>

  #define BOARD_NAME "esp8266"
  #define DATA_PIN   2           // GPIO2, marked D4 on NodeMCU and Wemos D1 mini (same as WLED)

  using WebServer = ESP8266WebServer;
  static ESP8266HTTPUpdate& httpUpdate = ESPhttpUpdate;

  inline void setHttpTimeouts(HTTPClient& http) { http.setTimeout(5000); }
  inline void startNtp(const char* tz, const char* server) { configTime(tz, server); }

  // ESP8266 has no Preferences library. This small stand-in has the same methods the firmware
  // uses and stores each setting as a file in LittleFS (formatted automatically on first start).
  class Preferences {
   public:
    bool begin(const char* ns, bool readOnly = false) {
      (void)readOnly;
      dir_ = String("/") + ns;
      if (!LittleFS.begin()) return false;
      LittleFS.mkdir(dir_);
      return true;
    }
    String getString(const char* key, const String& def = String()) {
      File f = LittleFS.open(path(key), "r");
      if (!f) return def;
      String s = f.readString();
      f.close();
      return s;
    }
    size_t putString(const char* key, const String& value) {
      File f = LittleFS.open(path(key), "w");
      if (!f) return 0;
      size_t n = f.print(value);
      f.close();
      return n;
    }
    int32_t getInt(const char* key, int32_t def = 0) {
      String s = getString(key);
      return s.length() ? (int32_t)s.toInt() : def;
    }
    size_t putInt(const char* key, int32_t value) { return putString(key, String(value)); }
    bool getBool(const char* key, bool def = false) {
      String s = getString(key);
      return s.length() ? s == "1" : def;
    }
    size_t putBool(const char* key, bool value) { return putString(key, value ? "1" : "0"); }

   private:
    String path(const char* key) { return dir_ + "/" + key; }
    String dir_;
  };

#else
  #error "Elpris-LED supports ESP32 and ESP8266 boards"
#endif
