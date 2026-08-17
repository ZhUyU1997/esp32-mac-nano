#!/usr/bin/env node
// idf-mcp: ESP-IDF console driven by an MCP agent and/or a human.
//
// The output area is a REAL terminal: the child pty is streamed raw into a
// DEC scroll region (rows 1..N-3) on the PRIMARY screen, so the terminal's
// native scrollback, text selection and copy all work exactly like a normal
// terminal. Mouse reporting is intentionally NOT enabled — that is what
// keeps native selection intact. The bottom header (status + buttons) is
// keyboard-driven.
//
// Layout:
//   rows 1..N-3  = embedded terminal (raw pty passthrough, native scrollback)
//   row  N-2     ┌─ status ────────────────────────────┐
//   row  N-1     │ buttons (←/→ + Enter, keyboard)    │
//   row  N       └────────────────────────────────────┘
//
// The agent drives the exact same state machine over MCP. A single "current
// child process" slot is the natural mutex. No persistent bash: each command
// is its own pty child process (exact exit codes, no completion sentinel).
import pty from "node-pty";
import type { IPty } from "node-pty";
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StreamableHTTPServerTransport } from "@modelcontextprotocol/sdk/server/streamableHttp.js";
import { z } from "zod";
import express, { type Request, type Response } from "express";
import http from "node:http";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { randomUUID } from "node:crypto";

const PORT = Number(process.env.MCP_IDF_PORT ?? 8765);
const HOST = process.env.MCP_IDF_HOST ?? "127.0.0.1";
const RING_MAX = Number(process.env.MCP_IDF_BUFFER ?? 16 * 1024 * 1024);
const MAX_LINE = 4096;
const MOCK = process.env.MCP_IDF_MOCK === "1";
const SERIAL_PORT = process.env.MCP_IDF_SERIAL_PORT ?? "";
const __dirname = path.dirname(fileURLToPath(import.meta.url));
// mock scripts live at the project root (../mock when running from dist/)
const MOCK_DIR = fs.existsSync(path.join(__dirname, "mock")) ? path.join(__dirname, "mock") : path.join(__dirname, "..", "mock");
const MOCK_FLASH = path.join(MOCK_DIR, "mock-flash.mjs");
const MOCK_MONITOR = path.join(MOCK_DIR, "mock-monitor.mjs");
const MOCK_FLASH_MONITOR = path.join(MOCK_DIR, "mock-flash-monitor.mjs");
const MOCK_BUILD_FLASH_MONITOR = path.join(MOCK_DIR, "mock-build-flash-monitor.mjs");

// ---------------------------------------------------------------------------
// IDF environment activation (captured ONCE, no persistent shell)
// ---------------------------------------------------------------------------
const ACTIVATE_DEFAULT = path.join(os.homedir(), ".espressif", "tools", "activate_idf_v5.5.4.sh");
let activateScript: string | null = null;
{
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--activate" && i + 1 < argv.length) activateScript = argv[i + 1];
  }
  if (activateScript === null && fs.existsSync(ACTIVATE_DEFAULT)) activateScript = ACTIVATE_DEFAULT;
}

let baseEnv: NodeJS.ProcessEnv = { ...process.env };
if (activateScript) {
  try {
    const raw = execFileSync("bash", ["-c", `source "${activateScript}" >/dev/null 2>&1; env -0`], {
      encoding: "utf8",
      env: process.env,
    });
    for (const part of raw.split("\0")) {
      const eq = part.indexOf("=");
      if (eq > 0) baseEnv[part.slice(0, eq)] = part.slice(eq + 1);
    }
  } catch (err) {
    console.error(`[idf-mcp] env capture failed: ${err instanceof Error ? err.message : String(err)}`);
  }
}

const projectDir = (() => {
  if (process.env.MCP_IDF_PROJECT) return process.env.MCP_IDF_PROJECT;
  let dir = process.cwd();
  for (let i = 0; i < 4; i++) {
    if (fs.existsSync(path.join(dir, "CMakeLists.txt"))) return dir;
    dir = path.dirname(dir);
  }
  return process.cwd();
})();
const projectName = path.basename(projectDir);

// ---------------------------------------------------------------------------
// output ring buffer (machine-readable view for the agent)
// ---------------------------------------------------------------------------
const ANSI_RE = /\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07/g;
const IDF_LEVEL_RE = /^([IWE]) \(\d+\)/;

let lastLines: string[] = [];
let lastBytes = 0;
let pending = "";
let lastCmdName: string | null = null;
let lastExit: number | null = null;
let lineSeq = 0;

function capLine(ln: string): string {
  const b = Buffer.from(ln);
  if (b.length <= MAX_LINE) return ln;
  return b.subarray(0, MAX_LINE).toString("utf8") + "…[truncated]";
}

function pushOutput(data: string): void {
  pending += data;
  let nl: number;
  while ((nl = pending.indexOf("\n")) !== -1) {
    let ln = pending.slice(0, nl);
    pending = pending.slice(nl + 1);
    if (ln.endsWith("\r")) ln = ln.slice(0, -1);
    ln = capLine(ln.replace(ANSI_RE, ""));
    lastLines.push(ln);
    lastBytes += Buffer.byteLength(ln);
    checkLineForWaiters(ln, ++lineSeq);
    // flash_monitor/build_flash_monitor: monitor takeover = flash phase done.
    // Only these kinds may transition to attached; build/flash/execute output
    // mentioning the marker must never unlock the slot.
    if (child && !child.attached && (child.kind === "flash_monitor" || child.kind === "build_flash_monitor") && /Executing action: monitor/.test(ln)) child.attached = true;
    while (lastBytes > RING_MAX && lastLines.length > 1) lastBytes -= Buffer.byteLength(lastLines.shift()!);
  }
}

function drainPending(): void {
  if (!pending) return;
  const ln = capLine(pending.replace(ANSI_RE, ""));
  pending = "";
  lastLines.push(ln);
  lastBytes += Buffer.byteLength(ln);
  checkLineForWaiters(ln, ++lineSeq);
  while (lastBytes > RING_MAX && lastLines.length > 1) lastBytes -= Buffer.byteLength(lastLines.shift()!);
}

function checkLineForWaiters(ln: string, seq: number): void {
  if (matchWaiters.length === 0) return;
  for (const w of [...matchWaiters]) {
    if (w.settled || seq <= w.startSeq) continue;
    w.re.lastIndex = 0;
    if (w.re.test(ln)) {
      w.settled = true;
      w.resolve();
    }
  }
  matchWaiters = matchWaiters.filter((w) => !w.settled);
}

function clearOutput(): void {
  lastLines = [];
  lastBytes = 0;
  pending = "";
  lastCmdName = null;
  lastExit = null;
}

function safeRegex(pattern: string, flags?: string): RegExp {
  try {
    return new RegExp(pattern, flags);
  } catch {
    return new RegExp(pattern.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"), flags);
  }
}

interface ReadOpts {
  tail?: boolean;
  offset?: number;
  filter?: string;
  level?: "I" | "W" | "E";
  invert?: boolean;
  caseSensitive?: boolean;
  count?: boolean;
  context?: number;
}
function readOutput(opts: ReadOpts): { text: string; totalLines: number; nextOffset: number; hasMore: boolean } {
  drainPending();
  const invert = opts.invert ?? false;
  const re = opts.filter ? safeRegex(opts.filter, opts.caseSensitive === false ? "i" : undefined) : null;
  const matchedIdx: number[] = [];
  const hasFilter = Boolean(opts.filter || opts.level);
  if (hasFilter) {
    for (let i = 0; i < lastLines.length; i++) {
      const ln = lastLines[i];
      if (opts.level) {
        const m = IDF_LEVEL_RE.exec(ln);
        if (!m || m[1] !== opts.level) continue;
      }
      if (re) {
        re.lastIndex = 0;
        const m = re.test(ln);
        if (invert ? m : !m) continue;
      }
      matchedIdx.push(i);
    }
  } else {
    for (let i = 0; i < lastLines.length; i++) matchedIdx.push(i);
  }

  // grep -c: report the match count over the whole buffer, skip paging/context.
  if (opts.count) {
    return {
      text: `${matchedIdx.length} matching line(s) in ${lastLines.length} buffered line(s)`,
      totalLines: matchedIdx.length,
      nextOffset: 0,
      hasMore: false,
    };
  }

  // grep -A/-B/-C: expand each match to a window of surrounding lines.
  const ctx = opts.context ?? 0;
  let idxs = matchedIdx;
  if (ctx > 0 && hasFilter) {
    const seen = new Set<number>();
    for (const i of matchedIdx) {
      const lo = Math.max(0, i - ctx);
      const hi = Math.min(lastLines.length - 1, i + ctx);
      for (let j = lo; j <= hi; j++) seen.add(j);
    }
    idxs = [...seen].sort((a, b) => a - b);
  }

  const n = 100;
  const tail = opts.tail ?? true;
  let start: number;
  let end: number;
  if (tail) {
    end = idxs.length;
    start = Math.max(0, end - n);
  } else {
    start = opts.offset ?? 0;
    end = Math.min(idxs.length, start + n);
  }
  const slice = idxs.slice(start, end).map((i) => lastLines[i]);
  return { text: slice.join("\n"), totalLines: idxs.length, nextOffset: end, hasMore: end < idxs.length };
}

// ---------------------------------------------------------------------------
// TUI (raw mode + embedded terminal, no mouse reporting)
// ---------------------------------------------------------------------------
const isTTY = Boolean(process.stdin.isTTY && process.stdout.isTTY);
let termCols = process.stdout.columns || 80;
let termRows = process.stdout.rows || 24;
const HEADER_ROWS = 3;
function headerTop(): number {
  return termRows - HEADER_ROWS + 1;
}

const C = {
  reset: "\x1b[0m",
  dim: "\x1b[2m",
  green: "\x1b[32m",
  yellow: "\x1b[33m",
  red: "\x1b[31m",
  cyan: "\x1b[36m",
};

function fit(s: string, width: number): string {
  let out = "";
  let vis = 0;
  let i = 0;
  while (i < s.length && vis < width) {
    if (s.charCodeAt(i) === 0x1b) {
      const m = /^\x1b\[[0-9;?]*[a-zA-Z]/.exec(s.slice(i));
      if (m) {
        out += m[0];
        i += m[0].length;
        continue;
      }
    }
    out += s[i];
    i++;
    vis++;
  }
  return out;
}

function visLen(s: string): number {
  return s.replace(/\x1b\[[0-9;?]*[a-zA-Z]/g, "").length;
}

function barRow(left: string, content: string, fill: string, right: string): string {
  const inner = fit(content, termCols - 2);
  const pad = termCols - 2 - visLen(inner);
  return C.cyan + left + C.reset + inner + C.cyan + (pad > 0 ? fill.repeat(pad) : "") + right + C.reset;
}

function statusContent(): string {
  let state: string;
  if (child) {
    const secs = Math.round((Date.now() - child.start) / 1000);
    state = C.yellow + `▶ ${child.cmd} ${secs}s` + C.reset;
  } else if (lastResult) {
    const ok = lastResult.exit === 0;
    state = (ok ? C.green + "✓ " : C.red + "✗ ") + `${lastResult.cmd} (exit ${lastResult.exit})` + C.reset;
  } else {
    state = C.green + "● idle" + C.reset;
  }
  return `${state}  ${projectName}`;
}

interface ButtonDef {
  label: string;
  run: () => void;
  disabled: () => boolean;
}
const buttonDefs: ButtonDef[] = [];
let selected = 0;

function buttonContent(): string {
  let line = "";
  for (let i = 0; i < buttonDefs.length; i++) {
    const b = buttonDefs[i];
    let seg: string;
    if (i === selected) seg = `\x1b[46;30m ${b.label} \x1b[0m`;
    else if (b.disabled()) seg = C.dim + ` ${b.label} ` + C.reset;
    else seg = ` ${b.label} `;
    line += seg + (i < buttonDefs.length - 1 ? " " : "");
  }
  return line;
}

function drawBars(): void {
  if (!isTTY) return;
  const h = headerTop();
  const top = barRow("┌", ` ${statusContent()} `, "─", "┐");
  const mid = barRow("│", ` ${buttonContent()} `, " ", "│");
  const bot = barRow("└", "", "─", "┘");
  process.stdout.write(
    `\x1b7\x1b[${h};1H\x1b[K${top}\x1b[${h + 1};1H\x1b[K${mid}\x1b[${h + 2};1H\x1b[K${bot}\x1b8`,
  );
}

// Scroll region must start at row 1 so the terminal feeds its native scrollback.
function setScrollRegion(): void {
  if (!isTTY) return;
  const bottom = Math.max(1, termRows - HEADER_ROWS);
  process.stdout.write(`\x1b[1;${bottom}r\x1b[1;1H`);
}

// Scroll the current output into the terminal's native scrollback.
function clearOutputArea(): void {
  if (!isTTY) return;
  const n = Math.max(0, termRows - HEADER_ROWS);
  if (n > 0) {
    process.stdout.write(`\x1b[${n};1H`);
    process.stdout.write("\n".repeat(n));
  }
  process.stdout.write("\x1b[1;1H");
}

function setupTUI(): void {
  if (!isTTY) return;
  process.stdin.setRawMode(true);
  process.stdin.resume();
  process.stdout.write("\x1b[?25l\x1b[2J\x1b[H"); // hide cursor, clear, home
  drawBars();
  setScrollRegion();
}

function teardownTUI(): void {
  if (!isTTY) return;
  try {
    // reset scroll region + show cursor + clear screen + home (clean exit)
    process.stdout.write("\x1b[r\x1b[?25h\x1b[2J\x1b[H" + C.reset);
    process.stdin.setRawMode(false);
  } catch {
    /* already gone */
  }
}

function moveSelection(dir: 1 | -1): void {
  const n = buttonDefs.length;
  for (let step = 0; step < n; step++) {
    selected = (selected + dir + n) % n;
    if (!buttonDefs[selected].disabled()) break;
  }
  drawBars();
}

function pressSelected(): void {
  const b = buttonDefs[selected];
  if (b && !b.disabled()) b.run();
}

function redraw(): void {
  if (!isTTY) return;
  if (buttonDefs[selected]?.disabled()) {
    const n = buttonDefs.length;
    for (let i = 0; i < n; i++) {
      selected = (selected + 1) % n;
      if (!buttonDefs[selected].disabled()) break;
    }
  }
  drawBars();
}

// ---------------------------------------------------------------------------
// child process (single slot)
// ---------------------------------------------------------------------------
type Kind = "build" | "flash" | "build_flash_monitor" | "flash_monitor" | "monitor" | "execute";

let child: { cmd: string; kind: Kind; proc: IPty; start: number; attached: boolean } | null = null;
let lastResult: { cmd: string; exit: number | null; end: number } | null = null;
let exitWaiters: (() => void)[] = [];

interface MatchWaiter {
  re: RegExp;
  startSeq: number;
  resolve: () => void;
  settled: boolean;
}
let matchWaiters: MatchWaiter[] = [];

function startChild(cmd: string, kind: Kind, file: string, args: string[]): void {
  clearOutput();
  const rows = Math.max(1, termRows - HEADER_ROWS);
  const proc = pty.spawn(file, args, {
    name: "xterm-256color",
    cols: termCols,
    rows,
    cwd: projectDir,
    env: { ...baseEnv, TERM: "xterm-256color" },
  });
  child = { cmd, kind, proc, start: Date.now(), attached: kind === "monitor" };
  lastCmdName = cmd;
  proc.onData((data: string) => {
    if (isTTY) process.stdout.write(data); // raw passthrough into scroll region
    pushOutput(data);
  });
  proc.onExit(({ exitCode, signal }) => {
    if (!child) return;
    drainPending();
    const code = signal ? 128 + signal : exitCode;
    lastExit = code;
    lastResult = { cmd: child.cmd, exit: code, end: Date.now() };
    child = null;
    const ws = exitWaiters;
    exitWaiters = [];
    for (const w of ws) w();
    redraw();
    console.error(`[idf-mcp] done: ${lastResult.cmd} (exit ${lastResult.exit})`);
  });
  clearOutputArea();
  redraw();
  console.error(`[idf-mcp] start: ${cmd}`);
}

function waitForExit(timeoutMs: number): Promise<boolean> {
  return new Promise((resolve) => {
    if (!child) {
      resolve(true);
      return;
    }
    let settled = false;
    const finish = (ok: boolean): void => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      const i = exitWaiters.indexOf(waiter);
      if (i >= 0) exitWaiters.splice(i, 1);
      resolve(ok);
    };
    const timer = setTimeout(() => finish(false), timeoutMs);
    const waiter = (): void => finish(true);
    exitWaiters.push(waiter);
  });
}

type WaitOutcome = "matched" | "exited" | "timedOut";

function waitForMatch(pattern: string, timeoutMs: number, includePast = false): Promise<WaitOutcome> {
  return new Promise((resolve) => {
    const re = safeRegex(pattern);
    drainPending();
    if (includePast) {
      for (const ln of lastLines) {
        re.lastIndex = 0;
        if (re.test(ln)) {
          resolve("matched");
          return;
        }
      }
    }
    if (!child) {
      resolve("exited");
      return;
    }
    const startSeq = includePast ? -1 : lineSeq;
    let settled = false;
    const timer = setTimeout(() => finish("timedOut"), timeoutMs);
    const waiter: MatchWaiter = { re, startSeq, resolve: () => finish("matched"), settled: false };
    const onExit = (): void => finish("exited");
    const finish = (outcome: WaitOutcome): void => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      const mi = matchWaiters.indexOf(waiter);
      if (mi >= 0) matchWaiters.splice(mi, 1);
      const ei = exitWaiters.indexOf(onExit);
      if (ei >= 0) exitWaiters.splice(ei, 1);
      resolve(outcome);
    };
    matchWaiters.push(waiter);
    exitWaiters.push(onExit);
  });
}

function interruptCurrent(): void {
  if (!child) return;
  const isMonitor = child.kind === "monitor" || child.kind === "flash_monitor" || child.kind === "build_flash_monitor";
  const key = isMonitor ? "\x1d" : "\x03";
  console.error(`[idf-mcp] interrupt: ${isMonitor ? "Ctrl-]" : "Ctrl-C"} -> ${child.cmd}`);
  child.proc.write(key);
}

// ---------------------------------------------------------------------------
// command construction + actions
// ---------------------------------------------------------------------------
function portArgs(port?: string): string[] {
  const p = port ?? SERIAL_PORT;
  return p ? ["-p", p] : [];
}

interface SpawnSpec {
  cmd: string;
  kind: Kind;
  file: string;
  args: string[];
}

// The activate script defines `idf.py` as a shell FUNCTION, not an executable
// on PATH — spawn the real script via the venv python instead.
function idfArgs(sub: string[]): { file: string; args: string[] } {
  const idfPy = path.join(baseEnv.IDF_PATH ?? "", "tools", "idf.py");
  return { file: "python", args: [idfPy, ...sub] };
}

function specBuild(extra?: string): SpawnSpec {
  const sub = ["build"];
  if (extra) sub.push(...extra.split(/[ ]+/).filter(Boolean));
  const { file, args } = idfArgs(sub);
  return { cmd: `idf.py build${extra ? ` ${extra}` : ""}`, kind: "build", file, args };
}
function specFlash(port?: string): SpawnSpec {
  if (MOCK) return { cmd: "flash (mock)", kind: "flash", file: "node", args: [MOCK_FLASH] };
  const { file, args } = idfArgs(["flash", ...portArgs(port)]);
  return { cmd: "idf.py flash", kind: "flash", file, args };
}
function specMonitor(port?: string): SpawnSpec {
  if (MOCK) return { cmd: "monitor (mock)", kind: "monitor", file: "node", args: [MOCK_MONITOR] };
  const { file, args } = idfArgs(["monitor", ...portArgs(port)]);
  return { cmd: "idf.py monitor", kind: "monitor", file, args };
}
function specFlashMonitor(port?: string): SpawnSpec {
  if (MOCK) return { cmd: "flash monitor (mock)", kind: "flash_monitor", file: "node", args: [MOCK_FLASH_MONITOR] };
  const { file, args } = idfArgs(["flash", "monitor", "--no-reset", ...portArgs(port)]);
  return { cmd: "idf.py flash monitor --no-reset", kind: "flash_monitor", file, args };
}
function specBuildFlashMonitor(port?: string): SpawnSpec {
  if (MOCK) return { cmd: "build flash monitor (mock)", kind: "build_flash_monitor", file: "node", args: [MOCK_BUILD_FLASH_MONITOR] };
  const { file, args } = idfArgs(["build", "flash", "monitor", "--no-reset", ...portArgs(port)]);
  return { cmd: "idf.py build flash monitor --no-reset", kind: "build_flash_monitor", file, args };
}
function specExecute(command: string): SpawnSpec {
  const wrapped = activateScript ? `source "${activateScript}" >/dev/null 2>&1; ${command}` : command;
  return { cmd: command, kind: "execute", file: "bash", args: ["-c", wrapped] };
}

function launch(spec: SpawnSpec): void {
  startChild(spec.cmd, spec.kind, spec.file, spec.args);
}

// --- button slot arbitration (mirrors withFreeSlot on the MCP side) ---
let slotAction = false;
function slotBlocked(): boolean {
  return !!child && !child.attached;
}
async function stopAttachedMonitor(): Promise<void> {
  if (!child || !child.attached) return;
  console.error(`[idf-mcp] auto-stop monitor: ${child.cmd} -> button`);
  interruptCurrent();
  await waitForExit(15_000);
}
function doWithSlot(fn: () => void): void {
  if (slotAction || slotBlocked()) return;
  slotAction = true;
  void (async () => {
    try {
      await stopAttachedMonitor();
      if (child) return;
      fn();
    } finally {
      slotAction = false;
      redraw();
    }
  })();
}
function doBuild(): void {
  doWithSlot(() => launch(specBuild()));
}
function doFlash(): void {
  doWithSlot(() => launch(specFlash()));
}
function doFlashMonitor(): void {
  doWithSlot(() => launch(specFlashMonitor()));
}
function doBuildFlashMonitor(): void {
  doWithSlot(() => launch(specBuildFlashMonitor()));
}
function doMonitor(): void {
  doWithSlot(() => launch(specMonitor()));
}
function doReboot(): void {
  if (child) {
    if (child.kind === "monitor") {
      console.error("[idf-mcp] reboot: reset via monitor (Ctrl-T Ctrl-R)");
      child.proc.write("\x14\x12");
    }
    return;
  }
  launch(specMonitor());
}
function doStop(): void {
  interruptCurrent();
}
function doClear(): void {
  clearOutputArea();
  if (!child) {
    lastResult = null;
    redraw();
  }
}
function doQuit(): void {
  quit();
}

buttonDefs.push(
  { label: "Build", run: doBuild, disabled: () => slotBlocked() },
  { label: "Flash", run: doFlash, disabled: () => slotBlocked() },
  { label: "Flash+Mon", run: doFlashMonitor, disabled: () => slotBlocked() },
  { label: "All", run: doBuildFlashMonitor, disabled: () => slotBlocked() },
  { label: "Monitor", run: doMonitor, disabled: () => slotBlocked() },
  { label: "Reboot", run: doReboot, disabled: () => !!child && child.kind !== "monitor" },
  { label: "Stop", run: doStop, disabled: () => !child },
  { label: "Clear", run: doClear, disabled: () => false },
  { label: "Quit", run: doQuit, disabled: () => false },
);

if (isTTY) {
  setupTUI();
  process.stdin.on("data", (d: Buffer) => {
    const s = d.toString();
    if (s === "\x1b[C" || s === "\t") moveSelection(1);
    else if (s === "\x1b[D" || s === "\x1b[Z") moveSelection(-1);
    else if (s === "\r" || s === "\n" || s === " ") pressSelected();
    else if (s === "\x03") {
      if (child) interruptCurrent();
      else quit();
    }
  });

  process.stdout.on("resize", () => {
    termCols = process.stdout.columns || 80;
    termRows = process.stdout.rows || 24;
    if (child) child.proc.resize(termCols, Math.max(1, termRows - HEADER_ROWS));
    setScrollRegion();
    clearOutputArea();
    redraw();
  });

  setInterval(() => {
    if (child) drawBars();
  }, 1000);
} else {
  console.error(`[idf-mcp] MCP server: http://${HOST}:${PORT}/mcp`);
  console.error(`[idf-mcp] project: ${projectDir}  mock: ${MOCK}  activate: ${activateScript ?? "(none)"}  tty: false`);
}

// ---------------------------------------------------------------------------
// exit / signals
// ---------------------------------------------------------------------------
let exiting = false;
function quit(): void {
  if (exiting) return;
  exiting = true;
  if (child) {
    try {
      child.proc.kill();
    } catch {
      /* already gone */
    }
  }
  teardownTUI();
  process.stdout.write("\r\n[idf-mcp] ended\r\n");
  process.exit(0);
}

process.on("SIGTERM", quit);
process.on("SIGINT", () => {
  if (child) interruptCurrent();
  else quit();
});
process.on("exit", teardownTUI);

// ---------------------------------------------------------------------------
// blocking helper
// ---------------------------------------------------------------------------
const DEFAULT_TIMEOUT_MS = 600_000;
const TAIL_LINES = 10;

function busyError(): { content: { type: "text"; text: string }[]; isError: true } | null {
  if (!child) return null;
  return {
    content: [
      {
        type: "text",
        text: `a command is already running: ${child.cmd} — stop it first (idf_interrupt / Stop button)`,
      },
    ],
    isError: true,
  };
}

async function runBlocking(
  spec: SpawnSpec,
  timeoutMs?: number,
  tailLines: number = TAIL_LINES,
): Promise<{ content: { type: "text"; text: string }[] }> {
  launch(spec);
  const done = await waitForExit(timeoutMs ?? DEFAULT_TIMEOUT_MS);
  drainPending();
  const tail = lastLines.slice(-tailLines).join("\n");
  return {
    content: [
      { type: "text", text: tail || "(no output yet)" },
      {
        type: "text",
        text: JSON.stringify({
          status: done ? "done" : "running",
          command: lastCmdName,
          exit: done ? lastExit : null,
          timedOut: !done,
          totalLines: lastLines.length,
        }),
      },
    ],
  };
}

async function runAsyncWait(
  spec: SpawnSpec,
  pattern: string,
  timeoutMs: number,
  tailLines: number = TAIL_LINES,
): Promise<{ content: { type: "text"; text: string }[] }> {
  launch(spec);
  const outcome = await waitForMatch(pattern, timeoutMs);
  drainPending();
  const tail = lastLines.slice(-tailLines).join("\n");
  return {
    content: [
      { type: "text", text: tail || "(no output yet)" },
      {
        type: "text",
        text: JSON.stringify({
          status: outcome,
          command: lastCmdName,
          matched: outcome === "matched" ? pattern : null,
          exit: outcome === "exited" ? lastExit : null,
          running: child?.cmd ?? null,
          totalLines: lastLines.length,
        }),
      },
    ],
  };
}

// ---------------------------------------------------------------------------
// flash+monitor tools: block until the monitor is attached (flash phase done), so
// the return alone means flashing finished — no extra idf_wait_for needed. An
// optional wait regex keeps blocking for a later marker (e.g. app_main).
// ---------------------------------------------------------------------------
type ToolResult = { content: { type: "text"; text: string }[]; isError?: true };

async function waitForAttached(timeoutMs: number): Promise<WaitOutcome> {
  if (!child) return "exited";
  if (child.attached) return "matched";
  return waitForMatch("Executing action: monitor", timeoutMs);
}

async function runFlashMonitor(spec: SpawnSpec, wait: string | undefined, timeoutMs: number, tailLines: number = TAIL_LINES): Promise<ToolResult> {
  launch(spec);
  let outcome = await waitForAttached(timeoutMs);
  let matched: string | null = outcome === "matched" ? "Executing action: monitor" : null;
  if (outcome === "matched" && wait) {
    outcome = await waitForMatch(wait, timeoutMs);
    matched = outcome === "matched" ? wait : null;
  }
  drainPending();
  const tail = lastLines.slice(-tailLines).join("\n");
  return {
    content: [
      { type: "text", text: tail || "(no output yet)" },
      {
        type: "text",
        text: JSON.stringify({
          status: outcome,
          command: lastCmdName,
          matched,
          exit: outcome === "exited" ? lastExit : null,
          running: child?.cmd ?? null,
          totalLines: lastLines.length,
        }),
      },
    ],
  };
}

// ---------------------------------------------------------------------------
// slot arbitration: an ATTACHED monitor is passive (just watching logs), so any
// new command auto-stops it (Ctrl-]) and launches immediately — no separate
// idf_interrupt step. While flash_monitor/build_flash_monitor are still flashing
// (not attached), and for build/flash/execute in progress, new commands are
// hard-busy: aborting an in-progress flash is refused.
// ---------------------------------------------------------------------------
async function withFreeSlot(run: () => Promise<ToolResult>): Promise<ToolResult> {
  if (!child) return run();
  const busy = busyError()!;
  if (!child.attached) return busy;
  console.error(`[idf-mcp] auto-stop monitor: ${child.cmd} -> new command`);
  interruptCurrent();
  if (!(await waitForExit(15_000))) return busy;
  return run();
}

// ---------------------------------------------------------------------------
// MCP server
// Shared so both the initialize-handshake `instructions` field and the
// idf://instructions resource stay in sync.
const SERVER_INSTRUCTIONS = `You drive an ESP-IDF console: ONE command slot, shared with the human
(same TUI buttons). Check idf_status first. An attached monitor is passive — any new
command auto-stops it (Ctrl-]) and starts; flashing in progress and build/flash/
execute are hard-busy ("a command is already running").

WORKFLOW — one-shot
- idf_build_flash_monitor (All): build + flash + attach monitor (--no-reset, one
  reset). idf_flash_monitor (Flash+Mon): same, no build. Both BLOCK until the monitor
  is attached ("Executing action: monitor") — return means flashing done; pass
  wait: "app_main" (+timeoutMs) to also wait for app start.
- Split steps only if needed: idf_build (BLOCKING, exit==0) -> idf_flash (BLOCKING)
  -> idf_monitor (ASYNC).

WAITING — never sleep
- Markers: "Hard resetting via RTS pin" flash done | "Executing action: monitor"
  attached | "app_main" app booted.
- idf_wait_for(pattern, timeoutMs): timeoutMs REQUIRED; returns matched line /
  exited / timedOut. Forward-only by default; includePast:true scans history (may
  match stale lines from earlier runs).

OBSERVING
- idf_read_output: last 100 lines; filter regex with invert/caseSensitive/count/
  context (grep-style), level:"E" for errors. Ring buffer 16 MB — old lines
  evicted. idf_log_stats: I/W/E counts. idf_status: running / last exit.
- Timeouts (600 s) leave the process running — keep observing, don't start a second
  command. idf_reboot: reset via monitor (async; confirm with idf_wait_for("app_main"));
  starts a monitor when idle.`;

// ---------------------------------------------------------------------------
function createServer(): McpServer {
  const mcp = new McpServer(
    { name: "idf-mcp", version: "0.8.0" },
    { instructions: SERVER_INSTRUCTIONS },
  );

  mcp.resource(
    "idf instructions",
    "idf://instructions",
    { description: "Task instructions for the agent driving this ESP-IDF console.", mimeType: "text/plain" },
    async (uri) => ({
      contents: [
        {
          uri: uri.toString(),
          mimeType: "text/plain",
          text: SERVER_INSTRUCTIONS,
        },
      ],
    }),
  );

  mcp.tool(
    "idf_execute",
    "Run an arbitrary shell command (bash -c) in the IDF project dir. BLOCKING. Only for steps without a dedicated tool; prefer the dedicated idf_* tools.",
    {
      command: z.string().describe("Arbitrary shell command to run (bash -c) in the IDF project."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for completion in ms (default 600000)."),
      tail: z.number().int().min(1).max(100).describe("How many tail lines of output to return."),
    },
    async ({ command, timeoutMs, tail }) => {
      return withFreeSlot(() => runBlocking(specExecute(command), timeoutMs, tail));
    },
  );

  mcp.tool(
    "idf_build",
    "Build the ESP-IDF project (idf.py build). BLOCKING, exit==0 on success. Call before flashing or after source changes.",
    {
      extra: z.string().regex(/^[\w\-.= /]+$/, "extra may only contain idf.py build flags").optional().describe("Extra idf.py build arguments."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for completion in ms."),
      tail: z.number().int().min(1).max(100).describe("How many tail lines of output to return."),
    },
    async ({ extra, timeoutMs, tail }) => {
      return withFreeSlot(() => runBlocking(specBuild(extra), timeoutMs, tail));
    },
  );

  mcp.tool(
    "idf_flash",
    "Flash the built firmware (idf.py flash). BLOCKING; output \"Hard resetting via RTS pin\" marks flashing done.",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for completion in ms."),
      tail: z.number().int().min(1).max(100).describe("How many tail lines of output to return."),
    },
    async ({ port, timeoutMs, tail }) => {
      return withFreeSlot(() => runBlocking(specFlash(port), timeoutMs, tail));
    },
  );

  mcp.tool(
    "idf_monitor",
    "Attach the serial monitor (async, returns immediately), or block with wait until a regex appears. For the common build+flash+log flow prefer idf_flash_monitor / idf_build_flash_monitor.",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
      wait: z.string().min(1).optional().describe("Regex; block until it appears in output (e.g. app_main to wait for the app to start)."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for `wait` in ms; required when wait is set."),
    },
    async ({ port, wait, timeoutMs }) => {
      const spec = specMonitor(port);
      if (wait) {
        if (timeoutMs == null) {
          return { content: [{ type: "text", text: "timeoutMs is required when wait is set" }], isError: true };
        }
        return withFreeSlot(() => runAsyncWait(spec, wait, timeoutMs));
      }
      return withFreeSlot(async () => {
        launch(spec);
        return { content: [{ type: "text", text: `started: ${spec.cmd}` }] };
      });
    },
  );

  mcp.tool(
    "idf_flash_monitor",
    "Flash then attach the monitor (one-shot, no build). BLOCKS until the monitor is attached (\"Executing action: monitor\") — return means flashing done; pass wait: \"app_main\" to also wait for app start.",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
      wait: z.string().min(1).optional().describe('Regex to wait for AFTER the monitor is attached (default: the call blocks until the monitor is attached = flashing done, then returns). e.g. "app_main" for app start.'),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait in ms for the attach phase, and for wait when set. Required when wait is set (default 600000)."),
    },
    async ({ port, wait, timeoutMs }) => {
      if (wait && timeoutMs == null) {
        return { content: [{ type: "text", text: "timeoutMs is required when wait is set" }], isError: true };
      }
      return withFreeSlot(() => runFlashMonitor(specFlashMonitor(port), wait, timeoutMs ?? DEFAULT_TIMEOUT_MS));
    },
  );

  mcp.tool(
    "idf_build_flash_monitor",
    "Build, flash, and attach the monitor in one go — the recommended one-shot flow. BLOCKS until the monitor is attached; pass wait: \"app_main\" to also wait for app start.",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
      wait: z.string().min(1).optional().describe('Regex to wait for AFTER the monitor is attached (default: the call blocks until the monitor is attached = flashing done, then returns). e.g. "app_main" for app start.'),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait in ms for the attach phase, and for wait when set. Required when wait is set (default 600000)."),
    },
    async ({ port, wait, timeoutMs }) => {
      if (wait && timeoutMs == null) {
        return { content: [{ type: "text", text: "timeoutMs is required when wait is set" }], isError: true };
      }
      return withFreeSlot(() => runFlashMonitor(specBuildFlashMonitor(port), wait, timeoutMs ?? DEFAULT_TIMEOUT_MS));
    },
  );

  mcp.tool(
    "idf_reboot",
    "Reset the chip: sends Ctrl-T Ctrl-R via the attached monitor, or starts a monitor that resets the chip on startup when idle.",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
    },
    async ({ port }) => {
      if (child) {
        if (child.kind === "monitor") {
          child.proc.write("\x14\x12");
          return { content: [{ type: "text", text: "sent reset via monitor (Ctrl-T Ctrl-R)" }] };
        }
        return busyError()!;
      }
      const spec = specMonitor(port);
      launch(spec);
      return { content: [{ type: "text", text: `started monitor (resets the chip on startup): ${spec.cmd}` }] };
    },
  );

  mcp.tool("idf_interrupt", "Send an interrupt (Ctrl-C / Ctrl-]) to the running command or monitor. Use to stop a build/flash or detach a monitor.", {}, async () => {
    const cmd = child?.cmd ?? null;
    interruptCurrent();
    return {
      content: [{ type: "text", text: cmd ? `sent interrupt to: ${cmd}` : "nothing running" }],
    };
  });

  mcp.tool(
    "idf_read_output",
    "Read lines from the captured output ring buffer (16 MB, oldest evicted). grep-style filtering: filter regex with invert (-v), caseSensitive (set false for -i), count (-c), context (surrounding lines); level:\"E\" for errors. Use idf_log_stats for level counts.",
    {
      tail: z.boolean().optional().describe("Return the most recent lines (default true); false reads from offset"),
      offset: z.number().int().min(0).optional().describe("Line index to start from (used when tail=false)"),
      filter: z.string().optional().describe("Regex filter on line content"),
      level: z.enum(["I", "W", "E"]).optional().describe("Filter by ESP-IDF log level prefix"),
      invert: z.boolean().optional().describe("grep -v: exclude lines matching the filter (default false)"),
      caseSensitive: z.boolean().optional().describe("Case-sensitive matching (default true; set false for grep -i)"),
      count: z.boolean().optional().describe("grep -c: return only the number of matching lines, skip content"),
      context: z.number().int().min(0).optional().describe("grep -A/-B/-C: include this many surrounding lines around each match (subject to normal tail/offset paging)"),
      clear: z.boolean().optional().describe("Clear the buffer after reading (default false)"),
    },
    async ({ tail, offset, filter, level, invert, caseSensitive, count, context, clear }) => {
      const r = readOutput({ tail, offset, filter, level, invert, caseSensitive, count, context });
      if (clear) clearOutput();
      return {
        content: [
          { type: "text", text: r.text || "(no output yet)" },
          {
            type: "text",
            text: JSON.stringify({
              command: lastCmdName,
              exit: lastExit,
              running: child?.cmd ?? null,
              totalLines: r.totalLines,
              returned: r.text ? r.text.split("\n").length : 0,
              nextOffset: r.nextOffset,
              hasMore: r.hasMore,
            }),
          },
        ],
      };
    },
  );

  mcp.tool("idf_log_stats", "Count I/W/E/other log levels in the captured output buffer and report last command status.", {}, async () => {
    drainPending();
    let info = 0;
    let warn = 0;
    let error = 0;
    let other = 0;
    for (const ln of lastLines) {
      const m = IDF_LEVEL_RE.exec(ln);
      if (!m) other++;
      else if (m[1] === "I") info++;
      else if (m[1] === "W") warn++;
      else error++;
    }
    return {
      content: [
        {
          type: "text",
          text: JSON.stringify(
            { command: lastCmdName, exit: lastExit, running: child?.cmd ?? null, totalLines: lastLines.length, bytes: lastBytes, levels: { info, warn, error, other } },
            null,
            2,
          ),
        },
      ],
    };
  });

  mcp.tool("idf_status", "Report console state: running command, last command + exit code, buffer size. Check before starting any new command — one slot shared with the human.", {}, async () => {
    return {
      content: [
        {
          type: "text",
          text: JSON.stringify(
            {
              projectDir,
              mock: MOCK,
              running: child ? { cmd: child.cmd, kind: child.kind, seconds: Math.round((Date.now() - child.start) / 1000), attached: child.attached } : null,
              lastCmd: lastResult ? { cmd: lastResult.cmd, exit: lastResult.exit, secondsAgo: Math.round((Date.now() - lastResult.end) / 1000) } : null,
              bufferBytes: lastBytes,
              bufferLines: lastLines.length,
            },
            null,
            2,
          ),
        },
      ],
    };
  });

  mcp.tool(
    "idf_wait_for",
    "Block until a regex matches the current command's output (timeoutMs required; forward-only unless includePast). Use to wait for app_main after flashing.",
    {
      pattern: z.string().min(1).describe("Regex to wait for in the current command's output."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).describe("Max wait in ms (required)."),
      includePast: z.boolean().optional().describe("Also match lines already in the buffer before this call (default false = forward-only)."),
    },
    async ({ pattern, timeoutMs, includePast }) => {
      const outcome = await waitForMatch(pattern, timeoutMs, includePast);
      if (outcome === "matched") {
        const re = safeRegex(pattern);
        let line: string | null = null;
        for (const ln of lastLines) {
          re.lastIndex = 0;
          if (re.test(ln)) line = ln;
        }
        return { content: [{ type: "text", text: line ?? "(no output)" }] };
      }
      return { content: [{ type: "text", text: outcome }] };
    },
  );

  return mcp;
}

// ---------------------------------------------------------------------------
// Streamable HTTP MCP server
// ---------------------------------------------------------------------------
const app = express();
app.use(express.json());
const transports = new Map<string, StreamableHTTPServerTransport>();

async function handleErr(res: Response, fn: () => Promise<void>): Promise<void> {
  try {
    await fn();
  } catch (err) {
    console.error(`[idf-mcp] handler error: ${err instanceof Error ? err.message : String(err)}`);
    if (!res.headersSent) res.status(500).end();
  }
}

app.post("/mcp", (req: Request, res: Response) =>
  handleErr(res, async () => {
    const sessionId = req.headers["mcp-session-id"];
    let transport = sessionId ? transports.get(String(sessionId)) : undefined;
    if (!transport) {
      const id = randomUUID();
      const mcp = createServer();
      transport = new StreamableHTTPServerTransport({
        sessionIdGenerator: () => id,
        onsessioninitialized: (sid) => {
          transports.set(sid, transport!);
        },
      });
      await mcp.connect(transport);
    }
    await transport.handleRequest(req, res, req.body);
  }),
);

app.get("/mcp", (req: Request, res: Response) =>
  handleErr(res, async () => {
    const sessionId = req.headers["mcp-session-id"];
    const transport = sessionId ? transports.get(String(sessionId)) : undefined;
    if (!transport) {
      res.status(405).end();
      return;
    }
    await transport.handleRequest(req, res);
  }),
);

app.delete("/mcp", (req: Request, res: Response) =>
  handleErr(res, async () => {
    const sessionId = req.headers["mcp-session-id"];
    const transport = sessionId ? transports.get(String(sessionId)) : undefined;
    if (!transport) {
      res.status(405).end();
      return;
    }
    await transport.handleRequest(req, res);
    transports.delete(String(sessionId));
  }),
);

http.createServer(app).listen(PORT, HOST);
