export const FILES = {
  "ESP32": "firmware/esp32-arduino.pico.merged.bin",
  "ESP32-S2": "firmware/esp32-arduino.s2.merged.bin",
  "ESP32-S3": "firmware/esp32-arduino.s3.merged.bin",
  "ESP32-C3": "firmware/esp32-arduino.c3.merged.bin",
};

export const CHIP_OF = { pico: "ESP32", s2: "ESP32-S2", s3: "ESP32-S3", c3: "ESP32-C3" };
export const LAST = 6;
export const CONNECT = 3;
export const WRITE = 4;

export const state = {
  lang: "en",
  step: 0,
  board: "s2",
  flashed: false,
  connected: false,
  busy: false,
};

export const session = {
  port: null,
  transport: null,
  loader: null,
  nativeUsb: false,
  chipName: "",
  imageKey: "",
};

export const serialOk = typeof navigator !== "undefined" && "serial" in navigator;
