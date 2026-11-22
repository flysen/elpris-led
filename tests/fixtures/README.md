# Test fixtures

`prices-2026-11-22-SE3.json` is a day of prices in the format of the elprisetjustnu.se API
(`/api/v1/prices/2026/11-22_SE3.json`): 96 quarters with the real SEK prices for SE3 on 2026-11-22, as
published on the repository's `prices` branch. `EUR_per_kWh` and `EXR` are filled in with a fixed exchange
rate; the lamp doesn't use them. It is about the same size as a real answer (13.4 kB here, 13.6 kB from the server), which is what the lamp
read only 571 bytes of in v5.0-v5.1.

To replace it with a real download:

```bash
curl -s https://www.elprisetjustnu.se/api/v1/prices/2026/11-22_SE3.json > tests/fixtures/prices-2026-11-22-SE3.json
```

Then update the first and last prices in `testParsePricesRealDay()` in `tests/test_logic.cpp`.
