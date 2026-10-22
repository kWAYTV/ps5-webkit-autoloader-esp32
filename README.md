# PS5 WebKit Autoloader for ESP32

ESP32 SoftAP that serves [srbraboo/ps5-webkit-autoloader](https://github.com/srbraboo/ps5-webkit-autoloader) from LittleFS so the PS5 User's Guide can install it.

For devices you own. Homebrew and WebKit exploits can violate Sony’s terms. A console or PSN account can be banned. This repo does not bypass Sony enforcement.

| Chain | Firmware |
| --- | --- |
| poops | 7.00–12.00 |
| relapse | 7.00–13.60 except 9.05 and 11.40 |

Poops is the default when both are valid.

## Flash

Hold **BOOT**, plug into a PC, then use the [web flasher](https://percdotdev.github.io/ps5-webkit-autoloader-esp32/) (Chrome / Edge) or write a [release](https://github.com/percdotdev/ps5-webkit-autoloader-esp32/releases/latest) merged bin at `0x0`. Same srbraboo version updates `latest`. A new version adds a release.

| Board | Image | LED |
| --- | --- | --- |
| ESP32-S2 | `esp32-arduino.s2.merged.bin` | GPIO 15 |
| ESP32-S3 | `esp32-arduino.s3.merged.bin` | GPIO 21 |
| ESP32-PICO | `esp32-arduino.pico.merged.bin` | GPIO 10 |
| ESP32-C3 | `esp32-arduino.c3.merged.bin` | GPIO 8 |

Solid on boot. Heartbeat when ready. Flash on HTTP. Fast blink if the AP fails.

Release images drop native USB after boot. Move the board to the PS5.

## Use

1. PS5 Wi-Fi: `ESP32_PORTAL` / `12345678`
2. **Settings → Guide & Tips → User's Guide**
3. Wait for cache and install. Leave the board plugged in.
4. Payload Manager opens. Later boots: **Media → WebKit Autoloader**. Stay on `ESP32_PORTAL`.

Relapse needs the SoftAP DHCP lease. Poops does not need a network interface.

## Build

```sh
./install-deps.sh
python3 pack/pack_srbraboo.py --web /path/to/ps5-webkit-autoloader
make
```

Arduino-ESP32 **2.0.11** only. Images land in `build/{pico,s2,s3,c3}/`. `make debug` / `make debug-s2` keeps USB CDC.

## Credits

Started from [owendswang/ps5-webkit-autoloader-esp32](https://github.com/owendswang/ps5-webkit-autoloader-esp32). Payload: [srbraboo](https://github.com/srbraboo/ps5-webkit-autoloader), [itsPLK](https://github.com/itsPLK/ps5-webkit-autoloader). Chains: [slopkit](https://github.com/jordyidk/slopkit), [Relapse](https://github.com/ntfargo/Relapse-Exploit). Flasher: [esptool-js](https://github.com/espressif/esptool-js).

## License

Firmware and build: MIT. Bundled payloads keep their own terms. See [LICENSE](LICENSE).
