import test from "node:test";
import assert from "node:assert/strict";
import { classifyTurn, makeReport, STATES, EFFECTS } from "./codex_status_agent.mjs";

test("maps Codex turn states to the traffic-light state machine", () => {
  assert.deepEqual(classifyTurn({ status: "inProgress" }), {
    name: "working", state: STATES.WORKING, effect: EFFECTS.BREATHE,
  });
  assert.deepEqual(classifyTurn({ status: "interrupted" }), {
    name: "waiting", state: STATES.WAITING, effect: EFFECTS.BREATHE,
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

test("creates the documented eight-byte HID report", () => {
  assert.deepEqual(makeReport({ name: "waiting", state: STATES.WAITING, effect: EFFECTS.BREATHE }, 257, 96), [
    0, 2, 1, 96, 220, 150, 0, 1, 0,
  ]);
});
