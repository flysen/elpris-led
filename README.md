# Elpris-LED

En LED-slinga som visar dygnets elpriser i färg: grönt när elen är billig, rött när den är dyr. Lampan hämtar spotpriset för ditt elområde (SE1–SE4) från [elprisetjustnu.se](https://www.elprisetjustnu.se) och visar dygnet från midnatt till midnatt längs slingan. Lysdioden för just nu pulserar. När morgondagens priser har publicerats, vanligtvis efter lunch, visas båda dygnen.

**Installera direkt från webbläsaren: [flysen.github.io/elpris-led](https://flysen.github.io/elpris-led/)**

Du behöver inte kompilera något. Installationssidan flashar färdig firmware via USB i Chrome eller Edge.

## Det här behöver du

| Del | Kommentar |
|---|---|
| ESP32-utvecklingskort | En vanlig ESP32, till exempel ESP32-DevKitC eller ett kort med WROOM-32. ESP32-C3, -S2 och -S3 stöds inte av den färdiga firmwaren. |
| WS2812B-slinga, 5 V | Upp till 300 LED. Antalet ställs in efter installationen. |
| Nätaggregat, 5 V | Räkna med upp till 60 mA per LED på full vit ljusstyrka. Med standardinställningen drar slingan betydligt mindre, men aggregatet bör klara full ljusstyrka. |
| Resistor, 330–470 Ω | I dataledningen, så nära slingan som möjligt. |
| Kondensator, 1000 µF, minst 6,3 V | Över +5 V och GND vid slingans ingång. |
| USB-kabel med data | För installationen. Vissa kablar kan bara ladda. |

ESP32:an skickar data med 3,3 V. Det fungerar med de flesta WS2812B-slingor, men om färgerna flimrar eller blir fel kan en nivåomvandlare som 74AHCT125 hjälpa.

## Koppling

```
Nätaggregat 5 V          ESP32                 WS2812B-slinga (DIN-änden)
  +5V ───────┬──────────── 5V / VIN
             └──────────────────────────────── +5V
  GND ───────┬──────────── GND
             └──────────────────────────────── GND
                           GPIO16 ── 330 Ω ─── DIN

1000 µF över +5V och GND vid slingans ingång, minusbenet mot GND.
```

- Slingan har en riktning. Anslut i den ände där pilarna pekar bort från dig, märkt DIN.
- ESP32 och slinga måste ha gemensam jord.
- Ha inte USB och nätaggregatets 5 V anslutna till ESP32:an samtidigt, om du inte vet att kortet tål det. Installera först via USB och koppla sedan in aggregatet.
- För långa slingor, mata också in 5 V i andra änden så att färgerna inte blir rödare längre bort.

## Installera

1. Öppna [installationssidan](https://flysen.github.io/elpris-led/) i Chrome eller Edge på en dator.
2. Anslut ESP32:an med USB, klicka på *Installera på ESP32* och välj porten.
3. Välj att radera enheten vid första installationen.

Syns ingen port behöver datorn en drivrutin för kortets USB-krets, oftast CP210x eller CH340.

## Första start

1. Slingan pulserar blått och lampan startar ett eget wifi som heter **Elpris-LED-Config**. Anslut till det med mobilen.
2. Öppnas ingen sida automatiskt, gå till `http://192.168.4.1`.
3. Välj ditt wifi, skriv lösenordet och fyll i elområde, antal LED, nattläge och ljusstyrka. Spara.
4. Lampan ansluter till ditt nätverk, hämtar tid och priser och börjar visa färger.
5. Leta upp lampans IP-adress i routern och öppna den i webbläsaren. Där ändrar du inställningar senare. Sätt ett **OTA-lösenord** och ett **lösenord för konfigportalen** direkt. Innan dess är OTA avstängt och konfigportalen öppen.

Om lampan inte hittar ditt wifi, till exempel efter ett strömavbrott då routern startar långsamt, öppnar den konfigportalen i tre minuter och startar sedan om för att försöka igen.

## Vad färgerna betyder

| Det du ser | Betydelse |
|---|---|
| Grönt till rött | Billigaste till dyraste priset i perioden som visas. Skalan är relativ, så även en billig dag har röda timmar. |
| En lysdiod pulserar | Den aktuella kvarten. |
| Hela slingan pulserar blått | Lampan väntar på tid eller priser. |
| Hela slingan pulserar lila | Det finns inga priser för just nu och hämtningen misslyckas. |
| De tre första lysdioderna blinkar magenta | Hämtningen misslyckas, men lampan visar sparade priser så länge de räcker. |

Priserna är spotpriser exklusive moms, nätavgift och energiskatt.

## Uppdatera

Använd installationssidan igen och välj att *inte* radera, så behålls wifi och inställningar. Har du satt ett OTA-lösenord kan du också ladda upp filen `elpris-led-<version>-esp32-ota.bin` från [Releases](https://github.com/flysen/elpris-led/releases) trådlöst.

## Bygga själv

Vill du ändra koden, till exempel använda en annan datapinne, bygger du med [arduino-cli](https://arduino.github.io/arduino-cli/) eller Arduino IDE. Pinnen ställs in med `DATA_PIN` i `elpris_led/elpris_led.ino`.

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install FastLED ArduinoJson WiFiManager
arduino-cli compile --fqbn esp32:esp32:esp32 elpris_led
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 elpris_led
```

Logiken för tider, slots och nattläge ligger i `elpris_led/elpris_logic.h` och testas på en vanlig dator:

```bash
make test
```

Varje push kompileras och testas automatiskt i GitHub Actions.

## Släppa en ny version

För den som underhåller repot. En versionstagg bygger firmwaren, skapar en release och uppdaterar installationssidan:

```bash
git tag v2.1
git push origin v2.1
```

Engångsinställningar i repot innan första releasen:

1. *Settings → Pages*: sätt *Source* till *GitHub Actions*.
2. *Settings → Environments → github-pages*: lägg till en regel som tillåter taggar som matchar `v*`. Annars stoppas publiceringen eftersom den körs från en tagg.

## Licens

Se [LICENSE](LICENSE).
