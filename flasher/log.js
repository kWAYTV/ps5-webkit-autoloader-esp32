import { t } from "./i18n.js";

const SKIP_LOG = /^(Writing at 0x|Compressed \d|Uploading stub|Running stub|Stub running|Changing baud|Hard resetting|Connecting\.\.\.|Detecting chip|Leaving\.\.\.|Staying in)/i;

let logStart = 0;
let logLines = [];

function fmtBytes(n) {
  if (n >= 1048576) {
    const mb = n / 1048576;
    return (Number.isInteger(mb) ? String(mb) : mb.toFixed(2)) + " MB";
  }
  if (n >= 1024) {
    const kb = n / 1024;
    return (Number.isInteger(kb) ? String(kb) : kb.toFixed(1)) + " KB";
  }
  return n + " B";
}

function fmtSec(n) {
  return (n >= 10 ? String(Math.round(n)) : n.toFixed(1)) + " s";
}

function stamp() {
  const s = Math.max(0, Math.floor((Date.now() - (logStart || Date.now())) / 1000));
  return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, "0")}`;
}

function tidy(raw) {
  const s = String(raw).replace(/\r/g, "").replace(/\s+/g, " ").trim();
  if (!s || SKIP_LOG.test(s)) return null;
  const wrote = s.match(/^Wrote (\d+) bytes.*?in ([\d.]+) seconds/i);
  if (wrote) return { text: `${t("wrote")} ${fmtBytes(+wrote[1])} · ${fmtSec(+wrote[2])}`, kind: "hi" };
  if (/^Hash of data verified/i.test(s)) return { text: t("verified"), kind: "hi" };
  if (/^Chip is /i.test(s)) return { text: s.replace(/^Chip is /i, `${t("chip")}  `), kind: "hi" };
  if (/^MAC:/i.test(s)) return { text: s.replace(/^MAC:\s*/i, "MAC  ") };
  if (/^Features:/i.test(s)) return { text: s.replace(/^Features:\s*/i, "") };
  if (/^Crystal is /i.test(s)) return { text: s.replace(/^Crystal is /i, "") };
  return { text: s };
}

export function setBar(n) {
  const bar = document.getElementById("bar");
  const pct = document.getElementById("pct");
  if (!bar || !pct) return;
  const v = Math.max(0, Math.min(100, n));
  bar.style.width = v + "%";
  pct.textContent = Math.round(v) + "%";
}

export function clearLog() {
  logStart = Date.now();
  logLines = [];
  const el = document.getElementById("log");
  if (el) el.replaceChildren();
}

export function logLine(line, kind) {
  const row = typeof line === "object" && line && "text" in line ? line : tidy(line);
  if (!row) return;
  const time = stamp();
  const k = kind || row.kind || "";
  logLines.push(`${time}  ${row.text}`);
  const el = document.getElementById("log");
  if (!el) return;
  const item = document.createElement("div");
  item.className = "log-row" + (k ? ` ${k}` : "");
  const tm = document.createElement("time");
  tm.textContent = time;
  const sp = document.createElement("span");
  sp.textContent = row.text;
  item.append(tm, sp);
  el.append(item);
  el.scrollTop = el.scrollHeight;
}

export async function copyLog() {
  const btn = document.getElementById("copy-log");
  if (!btn) return;
  try {
    await navigator.clipboard.writeText(logLines.join("\n"));
    btn.textContent = t("copied");
    setTimeout(() => { if (btn) btn.textContent = t("copy"); }, 1200);
  } catch {
    btn.textContent = t("fail");
  }
}

export function fmtBytesLabel(n) {
  return fmtBytes(n);
}
