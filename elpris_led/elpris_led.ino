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

#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include
