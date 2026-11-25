#!/usr/bin/env node
/*
 * SL25 Codex status agent.
 *
 * The agent reads the local Codex turn projection (SQLite), maps it to a
 * small, stable state machine, and sends an 8-byte HID report to the lamp.
 * The HID transport is optional so the status logic can be tested without
 * hardware using --dry-run.
 */

import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { execFileSync } from "node:child_process";
import { createRequire } from "node:module";
import { DatabaseSync } from "node:sqlite";
import { fileURLToPath } from "node:url";

const require = createRequire(import.meta.url);

export const STATES = Object.freeze({
  OFFLINE: 0,
  WORKING: 1,
  WAITING: 2,
  READY: 3,
  ERROR: 4,
});

export const EFFECTS = Object.freeze({
  SOLID: 0,
  BREATHE: 1,
  BLINK: 2,
});

const COLORS = Object.freeze({
  offline: [0, 0, 0],
  working: [220, 150, 0],
  waiting: [220, 150, 0],
  ready: [0, 70, 0],
  error: [220, 0, 0],
});

const DEFAULT_INTERVAL_MS = 1000;
const DEFAULT_BRIGHTNESS = 96;
const FRAME_MS = 50;

function parseArgs(argv) {
  const options = {
    intervalMs: DEFAULT_INTERVAL_MS,
    brightness: DEFAULT_BRIGHTNESS,
    dryRun: false,
    once: false,
    verbose: false,
    stateFile: null,
    path: null,
    vid: null,
    pid: null,
  };

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    const next = () => {
      if (i + 1 >= argv.length) throw new Error(`${arg} needs a value`);
      i += 1;
      return argv[i];
    };
    if (arg === "--dry-run") options.dryRun = true;
    else if (arg === "--once") options.once = true;
    else if (arg === "--verbose") options.verbose = true;
    else if (arg === "--interval-ms") options.intervalMs = Number(next());
    else if (arg === "--brightness") options.brightness = Number(next());
    else if (arg === "--state-file") options.stateFile = next();
    else if (arg === "--path") options.path = next();
    else if (arg === "--vid") options.vid = Number(next());
    else if (arg === "--pid") options.pid = Number(next());
    else if (arg === "--help" || arg === "-h") options.help = true;
    else throw new Error(`unknown option: ${arg}`);
  }

  if (!Number.isFinite(options.intervalMs) || options.intervalMs < 200) {
    throw new Error("--interval-ms must be at least 200");
  }
  if (!Number.isFinite(options.brightness) || options.brightness < 0 || options.brightness > 255) {
    throw new Error("--brightness must be between 0 and 255");
  }
  if ((options.vid === null) !== (options.pid === null)) {
    throw new Error("--vid and --pid must be provided together");
  }
  return options;
}

export function classifyTurn(turn, codexRunning = true) {
  if (!codexRunning) return { name: "offline", state: STATES.OFFLINE, effect: EFFECTS.SOLID };
  if (!turn) return { name: "ready", state: STATES.READY, effect: EFFECTS.SOLID };

  switch (String(turn.status)) {
    case "inProgress":
      return { name: "working", state: STATES.WORKING, effect: EFFECTS.SOLID };
    case "failed":
      return { name: "error", state: STATES.ERROR, effect: EFFECTS.BLINK };
    case "interrupted":
      return { name: "waiting", state: STATES.WAITING, effect: EFFECTS.BLINK };
    case "completed":
      return { name: "ready", state: STATES.READY, effect: EFFECTS.SOLID };
    default:
      return { name: "waiting", state: STATES.WAITING, effect: EFFECTS.BLINK };
  }
}

function codexHome() {
  return process.env.CODEX_HOME || path.join(os.homedir(), ".codex");
}

function isCodexRunning() {
  if (process.platform !== "win32") return true;
  try {
    const output = execFileSync("tasklist.exe", ["/FO", "CSV", "/NH"], {
      encoding: "utf8",
      windowsHide: true,
      stdio: ["ignore", "pipe", "ignore"],
    });
    return output.split(/\r?\n/).some((line) => /"codex(?:\.exe)?"/i.test(line));
  } catch {
    // If process enumeration is blocked, do not falsely turn a connected
    // device off; the database remains the source of truth.
    return true;
  }
}

function latestTurn(home) {
  const dbPath = path.join(home, "thread_history_1.sqlite");
  if (!fs.existsSync(dbPath)) return null;

  let db;
  try {
    db = new DatabaseSync(dbPath, { readOnly: true });
    const statePath = path.join(home, "state_5.sqlite");
    const hasMetadata = fs.existsSync(statePath);
    if (hasMetadata) {
      db.exec("PRAGMA query_only = ON");
      db.prepare("ATTACH DATABASE ? AS metadata").run(statePath);
    }
    const turns = db.prepare(`
      SELECT t.thread_id, t.status
      FROM thread_turns AS t
      ${hasMetadata ? `LEFT JOIN metadata.threads AS m ON m.id = t.thread_id
        WHERE COALESCE(m.archived, 0) = 0
          AND COALESCE(m.source, '') NOT LIKE '%"subagent"%'` : ""}
      ORDER BY COALESCE(t.started_at, 0) DESC, t.rollout_ordinal DESC
    `);
    const lockDir = path.join(home, "thread-writer-locks");
    const hasLocks = fs.existsSync(lockDir);
    const seen = new Set();
    let latest = null;
    for (const turn of turns.iterate()) {
      if (seen.has(turn.thread_id)) continue;
      seen.add(turn.thread_id);
      // A crashed or closed chat can leave an inProgress row in history.
      if (turn.status === "inProgress" && hasLocks) {
        if (fs.existsSync(path.join(lockDir, turn.thread_id + ".lock"))) return turn;
        continue;
      }
      latest ??= turn;
      if (!hasLocks) break;
    }
    return latest;
  } catch (error) {
    return { status: "failed", error_json: JSON.stringify({ message: error.message }) };
  } finally {
    db?.close();
  }
}

function stateFromFile(filePath) {
  const payload = JSON.parse(fs.readFileSync(filePath, "utf8"));
  const name = String(payload.status || payload.state || "").toLowerCase();
  const aliases = {
    working: "working",
    running: "working",
    inprogress: "working",
    waiting: "waiting",
    approval: "waiting",
    input: "waiting",
    ready: "ready",
    idle: "ready",
    completed: "ready",
    error: "error",
    failed: "error",
    offline: "offline",
    off: "offline",
  };
  const normalized = aliases[name] || "waiting";
  if (normalized === "offline") {
    return { name: "offline", state: STATES.OFFLINE, effect: EFFECTS.SOLID };
  }
  return classifyTurn({ status: normalized === "working" ? "inProgress" : normalized === "error" ? "failed" : normalized === "waiting" ? "interrupted" : normalized === "ready" ? "completed" : "offline" }, true);
}

export function readCodexStatus(options = {}) {
  const home = options.home || codexHome();
  const running = options.codexRunning ?? isCodexRunning();
  if (options.stateFile && fs.existsSync(options.stateFile)) {
    try {
      return stateFromFile(options.stateFile);
    } catch {
      return { name: "error", state: STATES.ERROR, effect: EFFECTS.BLINK };
    }
  }
  return classifyTurn(latestTurn(home), running);
}

export function makeReport(status, sequence, brightness = DEFAULT_BRIGHTNESS) {
  const color = COLORS[status.name] || COLORS.offline;
  // node-hid requires the report-ID byte at index 0.  The HID descriptor has
  // no Report ID item, so the required value is zero; the firmware receives
  // the following eight-byte SL25 payload.
  // Rev B selects the LED color by wire state. Its WAITING palette is yellow.
  const wireState = status.state === STATES.WORKING ? STATES.WAITING : status.state;
  return [0, wireState, status.effect, brightness, ...color, sequence & 0xff, 0];
}

export function makeBlinkFrame(status, sequence, brightness = DEFAULT_BRIGHTNESS, elapsedMs = 0) {
  let on;
  if (status.state === STATES.WAITING) {
    const phase = elapsedMs % 2000;
    on = phase < 200 || (phase >= 400 && phase < 600);
  } else {
    const period = status.state === STATES.ERROR ? 500 : 1000;
    on = elapsedMs % period < period / 2;
  }
  return makeReport({ ...status, effect: EFFECTS.SOLID }, sequence, on ? brightness : 0);
}

function loadHid(options) {
  if (options.dryRun) return null;
  let hid;
  try {
    hid = require("node-hid");
  } catch {
    throw new Error("未找到 node-hid。先运行 npm install，或使用 --dry-run 做状态测试。");
  }
  if (options.path) return new hid.HID(options.path);
  if (options.vid === null || options.pid === null) {
    throw new Error("连接实物时必须指定 --vid 和 --pid，或者指定 --path。");
  }
  const device = hid.devices().find((item) => item.vendorId === options.vid && item.productId === options.pid);
  if (!device?.path) throw new Error(`找不到 HID 设备 VID=${options.vid.toString(16)} PID=${options.pid.toString(16)}`);
  return new hid.HID(device.path);
}

function printHelp() {
  console.log(`用法:
  node host/codex_status_agent.mjs --dry-run
  node host/codex_status_agent.mjs --vid 0xVVVV --pid 0xPPPP

颜色策略:
  黄灯常亮 = Codex 正在工作
  黄灯双闪 = 等待用户输入/审批，或任务被中断
  红灯快闪 = Codex 回合失败
  绿灯常亮 = Codex 在线且空闲/已完成
  熄灭     = Codex 不在线

可选参数: --once --verbose --interval-ms N --brightness 0..255
          --state-file FILE（用于外部集成或测试） --path HID_PATH`);
}

export async function run(options) {
  const device = loadHid(options);
  let sequence = 0;
  let last;
  let blinkStarted = 0;
  let frames;
  const writeFrame = () => {
    device.write(makeBlinkFrame(last, sequence++, options.brightness, performance.now() - blinkStarted));
  };
  const send = () => {
    const status = readCodexStatus(options);
    const changed = last?.name !== status.name;
    last = status;
    if (changed) {
      clearInterval(frames);
      frames = undefined;
      if (device && !options.once && status.effect === EFFECTS.BLINK) {
        blinkStarted = performance.now();
        frames = setInterval(writeFrame, FRAME_MS);
      }
    }
    if (frames) {
      if (changed) writeFrame();
    } else {
      const report = makeReport(status, sequence++, options.brightness);
      if (device) device.write(report);
      else if (options.verbose || changed) {
        console.log(JSON.stringify({ ...status, report, at: new Date().toISOString() }));
      }
    }
    if (device && options.verbose && changed) console.log(`状态: ${status.name}`);
  };

  send();
  if (options.once) {
    device?.close();
    return;
  }
  const timer = setInterval(send, options.intervalMs);
  const stop = () => {
    clearInterval(timer);
    clearInterval(frames);
    try {
      device?.write(makeReport({ name: "offline", state: STATES.OFFLINE, effect: EFFECTS.SOLID }, sequence++, 0));
      device?.close();
    } finally {
      process.exit(0);
    }
  };
  process.once("SIGINT", stop);
  process.once("SIGTERM", stop);
}

if (process.argv[1] && path.resolve(process.argv[1]) === path.resolve(fileURLToPath(import.meta.url))) {
  try {
    const options = parseArgs(process.argv.slice(2));
    if (options.help) printHelp();
    else await run(options);
  } catch (error) {
    console.error(`错误: ${error.message}`);
    process.exitCode = 1;
  }
}
