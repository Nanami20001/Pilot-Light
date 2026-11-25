import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { DatabaseSync } from "node:sqlite";
import { classifyTurn, makeReport, makeBlinkFrame, readCodexStatus, STATES, EFFECTS } from "./codex_status_agent.mjs";

test("each animated state has distinct on/off phases with no intermediate brightness", () => {
  const cases = [
    ["interrupted", [0, 200, 400, 600, 1999, 2000], [96, 0, 96, 0, 0, 96]],
    ["failed", [0, 249, 250, 499, 500], [96, 96, 0, 0, 96]],
  ];
  for (const [turn, times, expected] of cases) {
    const status = classifyTurn({ status: turn });
    const reports = times.map((ms, i) => makeBlinkFrame(status, i, 96, ms));
    assert.deepEqual(reports.map(report => report[3]), expected);
    assert.ok(reports.every(report => report[1] === status.state && report[2] === EFFECTS.SOLID));
  }
});

test("a live chat stays working across other completions and returns to ready when done", (t) => {
  const home = fs.mkdtempSync(path.join(os.tmpdir(), "sl25-"));
  t.after(() => fs.rmSync(home, { recursive: true, force: true }));
  const locks = path.join(home, "thread-writer-locks");
  fs.mkdirSync(locks);
  for (const id of ["chat", "guardian"]) fs.writeFileSync(path.join(locks, id + ".lock"), "");
  const metadata = new DatabaseSync(path.join(home, "state_5.sqlite"));
  metadata.exec(`CREATE TABLE threads(id TEXT, source TEXT, archived INTEGER);
    INSERT INTO threads VALUES ('guardian', '{"subagent":{"other":"guardian"}}', 0)`);
  metadata.close();
  const history = new DatabaseSync(path.join(home, "thread_history_1.sqlite"));
  try {
    history.exec(`CREATE TABLE thread_turns(thread_id TEXT, status TEXT, started_at INTEGER, rollout_ordinal INTEGER);
      INSERT INTO thread_turns VALUES ('chat','inProgress',1,1), ('other','completed',2,1),
      ('stale','inProgress',3,1), ('guardian','completed',4,1)`);
    const status = () => readCodexStatus({ home, codexRunning: true }).name;
    assert.equal(status(), "working");
    history.exec("UPDATE thread_turns SET status='completed' WHERE thread_id='chat'");
    history.exec("UPDATE thread_turns SET status='inProgress' WHERE thread_id='guardian'");
    assert.equal(status(), "ready");
  } finally {
    history.close();
  }
});

test("maps Codex turn states to the traffic-light state machine", () => {
  assert.deepEqual(classifyTurn({ status: "inProgress" }), {
    name: "working", state: STATES.WORKING, effect: EFFECTS.SOLID,
  });
  assert.deepEqual(classifyTurn({ status: "interrupted" }), {
    name: "waiting", state: STATES.WAITING, effect: EFFECTS.BLINK,
  });
  assert.deepEqual(classifyTurn({ status: "failed" }), {
    name: "error", state: STATES.ERROR, effect: EFFECTS.BLINK,
  });
  assert.deepEqual(classifyTurn({ status: "completed" }), {
    name: "ready", state: STATES.READY, effect: EFFECTS.SOLID,
  });
});

test("offline process state always wins", () => {
  assert.equal(classifyTurn({ status: "inProgress" }, false).name, "offline");
  assert.equal(classifyTurn(null, false).state, STATES.OFFLINE);
});

test("working uses the installed firmware's steady yellow output", () => {
  const status = classifyTurn({ status: "inProgress" });
  assert.deepEqual(makeReport(status, 0, 96), [0, 2, 0, 96, 220, 150, 0, 0, 0]);
});

test("creates the documented eight-byte HID report", () => {
  assert.deepEqual(makeReport({ name: "waiting", state: STATES.WAITING, effect: EFFECTS.BLINK }, 257, 96), [
    0, 2, 2, 96, 220, 150, 0, 1, 0,
  ]);
});
