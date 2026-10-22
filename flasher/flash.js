import { ESPLoader, Transport } from "https://unpkg.com/esptool-js@0.7.0/bundle.js";
import { FILES, CHIP_OF, state, session } from "./state.js";
import { t } from "./i18n.js";
import { logLine } from "./log.js";

function ignoreMissingSignals(transport) {
  const wrap = (fn) => async function (...args) {
    try { return await fn.apply(this, args); }
    catch (err) {
      if (String(err).includes("control signals")) return;
      throw err;
    }
  };
  transport.setRTS = wrap(transport.setRTS);
  transport.setDTR = wrap(transport.setDTR);
  transport.setSignals = wrap(transport.setSignals);
}

export async function dropSession() {
  try { await session.transport?.disconnect(); } catch { /* already gone */ }
  session.port = null;
  session.transport = null;
  session.loader = null;
  session.nativeUsb = false;
  session.chipName = "";
  session.imageKey = "";
  state.connected = false;
}

async function openPort() {
  try {
    return await navigator.serial.requestPort({
      filters: [
        { usbVendorId: 0x303a },
        { usbVendorId: 0x10c4 },
        { usbVendorId: 0x1a86 },
        { usbVendorId: 0x0403 },
      ],
    });
  } catch (err) {
    if (err?.name === "NotFoundError") throw Object.assign(new Error(t("noport")), { abort: true });
    throw err;
  }
}

export async function openSession(force) {
  if (session.loader && !force) return;
  await dropSession();
  const port = await openPort();
  session.port = port;
  session.nativeUsb = port.getInfo().usbVendorId === 0x303a;
  session.transport = new Transport(port, false);
  ignoreMissingSignals(session.transport);
  session.loader = new ESPLoader({
    transport: session.transport,
    baudrate: 115200,
    romBaudrate: 115200,
    terminal: {
      clean() {},
      writeLine: (d) => logLine(d),
      write() {},
    },
  });

  if (session.nativeUsb) logLine(t("native"));
  let chip;
  try {
    chip = await session.loader.main(session.nativeUsb ? "no_reset" : "default_reset");
  } catch (err) {
    if (!String(err).includes("control signals")) throw err;
    logLine(t("native"));
    try { await session.transport.disconnect(); } catch { /* already closed */ }
    chip = await session.loader.main("no_reset");
  }
  const name = session.loader.chip?.CHIP_NAME || chip || CHIP_OF[state.board];
  session.chipName = name;
  session.imageKey = FILES[name] ? name : CHIP_OF[state.board];
  state.connected = true;
  logLine(`${t("chip")}  ${name}`, "hi");
  if (CHIP_OF[state.board] !== session.imageKey) logLine(t("mismatch"));
  logLine(`${t("using")}  ${FILES[session.imageKey].split("/").pop()}`);
}

export async function loadFirmware(path, onProg) {
  const res = await fetch(`${path}?v=${Date.now()}`, { cache: "no-store" });
  if (!res.ok) throw new Error(`${path} (${res.status})`);
  onProg(10);
  const buf = new Uint8Array(await res.arrayBuffer());
  if (buf.length < 1024 || buf[0] === 0x3c) throw new Error(t("badbin"));
  onProg(40);
  return buf;
}

export async function writeImage(data, onProg) {
  await session.loader.writeFlash({
    fileArray: [{ data: new Uint8Array(data), address: 0 }],
    flashSize: "4MB",
    flashMode: "keep",
    flashFreq: "keep",
    eraseAll: false,
    compress: false,
    reportProgress(_i, written, total) {
      onProg(40 + (written / total) * 60);
    },
  });
  try { await session.loader.after("no_reset"); } catch { /* S2 CDC cannot leave download mode over USB */ }
}
