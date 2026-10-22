# PS5 WebKit Autoloader for ESP32

[English](#ps5-webkit-autoloader-for-esp32) · [简体中文](#esp32-版-ps5-webkit-autoloader)

This turns a cheap ESP32 board into a Wi-Fi hotspot. You plug the board into the PS5, join that hotspot, and open the official **User’s Guide**. The guide loads a page from the board (not from the internet). That page installs [srbraboo/ps5-webkit-autoloader](https://github.com/srbraboo/ps5-webkit-autoloader). After that, **Media → WebKit Autoloader** starts Payload Manager.

Use it on a console you own. Homebrew and WebKit exploits can violate Sony’s terms. The console or the PSN account can be banned if that console goes online. This project does not hide you from Sony.

| Exploit | PS5 system software |
| --- | --- |
| poops | 7.00–12.00 |
| relapse | 7.00–13.60 except 9.05 and 11.40 |

If both work on your version, poops is the one that runs.

## What you need

- A PS5 on one of the versions above
- An ESP32 board with a USB port: **ESP32-S2 Mini** (TENSTAR and similar), ESP32-S3, ESP32-C3, or ESP32-PICO
- A USB cable that carries data, not charge-only
- Chrome or Edge on a PC to write the firmware (Firefox and Safari cannot)

## Write the firmware (one time)

The board ships empty. You write our image onto it from a PC. After that you move the board to the PS5.

### Web flasher

1. Open the [web flasher](https://percdotdev.github.io/ps5-webkit-autoloader-esp32/) in Chrome or Edge.
2. Pick the chip that matches the metal shield on the board. A TENSTAR ESP32-S2 Mini is **ESP32-S2**.
3. Unplug the board. Hold the **BOOT** button, plug it into the PC, wait two seconds, then let go. That is download mode. Without it the chip ignores the write.
4. Select the port (a COM port, or “USB JTAG/serial debug unit”) and write. Leave the tab open.

When the write finishes the board may vanish from Windows. That is normal. These images turn off native USB after boot so the PS5 does not fight the serial port.

You can also download a [release](https://github.com/percdotdev/ps5-webkit-autoloader-esp32/releases/latest) and write the matching `.merged.bin` at address `0x0` with esptool. Same srbraboo version updates the `latest` release in place. A new srbraboo version gets its own release.

| Board | File | On-board LED |
| --- | --- | --- |
| ESP32-S2 | `esp32-arduino.s2.merged.bin` | GPIO 15 |
| ESP32-S3 | `esp32-arduino.s3.merged.bin` | GPIO 21 |
| ESP32-PICO | `esp32-arduino.pico.merged.bin` | GPIO 10 |
| ESP32-C3 | `esp32-arduino.c3.merged.bin` | GPIO 8 |

The LED stays on while the board boots, then does a short blink every couple of seconds when the hotspot is up. It flashes on each page request. Fast blinking means the Wi-Fi hotspot failed to start.

## Use it on the PS5

1. Unplug the board from the PC and plug it into a USB port on the PS5. Leave it there.
2. On the PS5: **Settings → Network → Settings → Set Up Internet Connection**. Join Wi-Fi **`ESP32_PORTAL`**, password **`12345678`**.
3. Stay on that network. The PS5 will say there is no internet. That is expected. There is no internet, and none is required.
4. **Settings → Guide and Tips → User’s Guide**. Wait until the page finishes loading. The first time this caches the files onto the console.
5. Payload Manager should open. A **WebKit Autoloader** shortcut appears under **Media**.

Next time you only need the board plugged in, Wi-Fi still on `ESP32_PORTAL`, then **Media → WebKit Autoloader**.

Relapse needs the IP the board hands out (the PS5 must stay on `ESP32_PORTAL`). Poops does not need a network interface.

## Build from source

```sh
./install-deps.sh
python3 pack/pack_srbraboo.py --web /path/to/ps5-webkit-autoloader
make
```

Arduino-ESP32 **2.0.11** only. Images land in `build/{pico,s2,s3,c3}/`. `make debug` / `make debug-s2` keeps USB serial so you can read logs.

## Credits

Started from [owendswang/ps5-webkit-autoloader-esp32](https://github.com/owendswang/ps5-webkit-autoloader-esp32). Payload: [srbraboo](https://github.com/srbraboo/ps5-webkit-autoloader), [itsPLK](https://github.com/itsPLK/ps5-webkit-autoloader). Chains: [slopkit](https://github.com/jordyidk/slopkit), [Relapse](https://github.com/ntfargo/Relapse-Exploit). Flasher: [esptool-js](https://github.com/espressif/esptool-js).

## License

Firmware and build: MIT. Bundled payloads keep their own terms. See [LICENSE](LICENSE).

---

# ESP32 版 PS5 WebKit Autoloader

[English](#ps5-webkit-autoloader-for-esp32) · [简体中文](#esp32-版-ps5-webkit-autoloader)

把一块便宜的 ESP32 变成 Wi-Fi 热点。把板子插到 PS5 上，连上这个热点，再打开官方的**用户指南**。指南会从板子加载页面（不是从互联网）。那个页面会安装 [srbraboo/ps5-webkit-autoloader](https://github.com/srbraboo/ps5-webkit-autoloader)。之后，**媒体 → WebKit Autoloader** 会启动 Payload Manager。

只在你自己的主机上使用。自制程序和 WebKit 漏洞可能违反索尼条款。主机或 PSN 账号如果联网，可能被封。这个项目不会帮你躲开索尼。

| 漏洞 | PS5 系统软件 |
| --- | --- |
| poops | 7.00–12.00 |
| relapse | 7.00–13.60，不包括 9.05 和 11.40 |

两个都能用时，实际跑的是 poops。

## 你需要什么

- 一台系统版本在上表范围内的 PS5
- 带 USB 口的 ESP32：**ESP32-S2 Mini**（TENSTAR 等同款）、ESP32-S3、ESP32-C3 或 ESP32-PICO
- 能传数据的 USB 线，不要只能充电的线
- 电脑上的 Chrome 或 Edge，用来写固件（Firefox 和 Safari 不行）

## 写入固件（只需一次）

板子出厂是空的。先在电脑上把镜像写进去，再拿到 PS5 上用。

### 网页烧录器

1. 用 Chrome 或 Edge 打开[网页烧录器](https://percdotdev.github.io/ps5-webkit-autoloader-esp32/?lang=zh)。
2. 选和金属屏蔽罩上芯片一致的型号。TENSTAR ESP32-S2 Mini 选 **ESP32-S2**。
3. 拔掉板子。按住 **BOOT** 键，插到电脑上，等两秒再松开。这就是下载模式。不这样做芯片会忽略写入。
4. 选端口（COM 口，或 “USB JTAG/serial debug unit”），然后写入。标签页不要关。

写完后板子可能从 Windows 里消失。正常。这些镜像启动后会关掉原生 USB，避免和 PS5 抢串口。

也可以从 [Releases](https://github.com/percdotdev/ps5-webkit-autoloader-esp32/releases/latest) 下载对应的 `.merged.bin`，用 esptool 写到地址 `0x0`。同一个 srbraboo 版本会原地更新 `latest`。新的 srbraboo 版本会单独发一个 Release。

| 板子 | 文件 | 板载 LED |
| --- | --- | --- |
| ESP32-S2 | `esp32-arduino.s2.merged.bin` | GPIO 15 |
| ESP32-S3 | `esp32-arduino.s3.merged.bin` | GPIO 21 |
| ESP32-PICO | `esp32-arduino.pico.merged.bin` | GPIO 10 |
| ESP32-C3 | `esp32-arduino.c3.merged.bin` | GPIO 8 |

开机时 LED 常亮，热点起来后每隔几秒短闪一次。每次页面请求会闪一下。快闪表示 Wi-Fi 热点没启动成功。

## 在 PS5 上使用

1. 从电脑上拔下板子，插到 PS5 的 USB 口。一直插着。
2. 在 PS5 上：**设置 → 网络 → 设置 → 设置互联网连接**。加入 Wi-Fi **`ESP32_PORTAL`**，密码 **`12345678`**。
3. 留在这个网络上。PS5 会提示没有互联网。正常。这个网络没有互联网，也不需要。
4. **设置 → 指南和提示 → 用户指南**。等到页面加载完。第一次会把文件缓存到主机上。
5. 应该会打开 Payload Manager。**媒体**里会出现 **WebKit Autoloader** 快捷方式。

之后只需要板子插着、Wi-Fi 仍连着 `ESP32_PORTAL`，然后 **媒体 → WebKit Autoloader**。

Relapse 需要板子分配的 IP（PS5 必须留在 `ESP32_PORTAL`）。Poops 不需要网卡。

## 从源码编译

```sh
./install-deps.sh
python3 pack/pack_srbraboo.py --web /path/to/ps5-webkit-autoloader
make
```

只支持 Arduino-ESP32 **2.0.11**。镜像在 `build/{pico,s2,s3,c3}/`。`make debug` / `make debug-s2` 会保留 USB 串口，方便看日志。

## 致谢

起步于 [owendswang/ps5-webkit-autoloader-esp32](https://github.com/owendswang/ps5-webkit-autoloader-esp32)。Payload：[srbraboo](https://github.com/srbraboo/ps5-webkit-autoloader)、[itsPLK](https://github.com/itsPLK/ps5-webkit-autoloader)。利用链：[slopkit](https://github.com/jordyidk/slopkit)、[Relapse](https://github.com/ntfargo/Relapse-Exploit)。烧录器：[esptool-js](https://github.com/espressif/esptool-js)。

## 许可

固件和构建脚本：MIT。附带的 payload 仍按各自条款。见 [LICENSE](LICENSE)。
