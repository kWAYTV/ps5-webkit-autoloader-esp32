# PS5 WebKit Autoloader for ESP32

ESP32 SoftAP host for [srbraboo/ps5-webkit-autoloader](https://github.com/srbraboo/ps5-webkit-autoloader). Firmware is [owendswang/ps5-webkit-autoloader-esp32](https://github.com/owendswang/ps5-webkit-autoloader-esp32) with that payload packed into LittleFS.

| Chain | Firmware |
| --- | --- |
| poops | 7.00–12.00 |
| relapse | 7.00–13.60 except 9.05 and 11.40 |

Default chain is poops when both are valid.

## Flash

Hold **BOOT**, plug into a PC, then use the [web flasher](https://kwaytv.github.io/ps5-webkit-autoloader-esp32/) (Chrome / Edge) or write a [release](https://github.com/kWAYTV/ps5-webkit-autoloader-esp32/releases/latest) merged bin at `0x0`.

| Board | Image |
| --- | --- |
| ESP32-S2 | `esp32-arduino.s2.merged.bin` |
| ESP32-S3 | `esp32-arduino.s3.merged.bin` |
| ESP32-PICO | `esp32-arduino.pico.merged.bin` |
| ESP32-C3 | `esp32-arduino.c3.merged.bin` |

Release images disconnect native USB after boot. Unplug and move the board to the PS5.

## Use

1. PS5 → Wi-Fi `ESP32_PORTAL` / `12345678`
2. **Settings → Guide & Tips → User's Guide**
3. Wait for cache and install. Leave the ESP32 plugged in.
4. Payload Manager opens. A **WebKit Autoloader** shortcut appears under Media.

Later boots: launch the Media shortcut. Stay on `ESP32_PORTAL`.

Relapse needs the PS5 to have an IP. The ESP32 SoftAP assigns one. There is no internet, and none is required. Poops does not need a network interface.

## Build

```sh
./install-deps.sh
python3 tools/pack_srbraboo.py --web /path/to/ps5-webkit-autoloader
make
```

Arduino-ESP32 **2.0.11** only. Use that core's `mklittlefs`, not 4.x. Images land in `build/{pico,s2,s3,c3}/`. `make debug` / `make debug-s2` keeps USB CDC.

## Credits

- [owendswang/ps5-webkit-autoloader-esp32](https://github.com/owendswang/ps5-webkit-autoloader-esp32)
- [srbraboo/ps5-webkit-autoloader](https://github.com/srbraboo/ps5-webkit-autoloader)
- [itsPLK/ps5-webkit-autoloader](https://github.com/itsPLK/ps5-webkit-autoloader)
- [itsPLK/ps5-unified-autoloader](https://github.com/itsPLK/ps5-unified-autoloader)
- [jordyidk/slopkit](https://github.com/jordyidk/slopkit)
- [ntfargo/Relapse-Exploit](https://github.com/ntfargo/Relapse-Exploit)
- [ESP Web Tools](https://esphome.github.io/esp-web-tools/)

## License

Firmware and build code: MIT. Bundled payloads and modified upstream keep their own terms. See [LICENSE](LICENSE).
