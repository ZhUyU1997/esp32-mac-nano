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
const MOCK_FLASH = path.join(__dirname, "mock", "mock-flash.mjs");
const MOCK_MONITOR = path.join(__dirname, "mock", "mock-monitor.mjs");
const MOCK_FLASH_MONITOR = path.join(__dirname, "mock", "mock-flash-monitor.mjs");
const MOCK_BUILD_FLASH_MONITOR = path.join(__dirname, "mock", "mock-build-flash-monitor.mjs");

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
    while (lastBytes > RING_MAX && lastLines.length > 1) lastBytes -= Buffer.byteLength(lastLines.shift()!);
  }
}

function drainPending(): void {
  if (!pending) return;
  const ln = capLine(pending.replace(ANSI_RE, ""));
  pending = "";
  lastLines.push(ln);
  lastBytes += Buffer.byteLength(ln);
  while (lastBytes > RING_MAX && lastLines.length > 1) lastBytes -= Buffer.byteLength(lastLines.shift()!);
}

function clearOutput(): void {
  lastLines = [];
  lastBytes = 0;
  pending = "";
  lastCmdName = null;
  lastExit = null;
}

function safeRegex(pattern: string): RegExp {
  try {
    return new RegExp(pattern);
  } catch {
    return new RegExp(pattern.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"));
  }
}

interface ReadOpts {
  tail?: boolean;
  offset?: number;
  filter?: string;
  level?: "I" | "W" | "E";
}
function readOutput(opts: ReadOpts): { text: string; totalLines: number; nextOffset: number; hasMore: boolean } {
  drainPending();
  const matchedIdx: number[] = [];
  const hasFilter = Boolean(opts.filter || opts.level);
  if (hasFilter) {
    const re = opts.filter ? safeRegex(opts.filter) : null;
    for (let i = 0; i < lastLines.length; i++) {
      const ln = lastLines[i];
      if (opts.level) {
        const m = IDF_LEVEL_RE.exec(ln);
        if (!m || m[1] !== opts.level) continue;
      }
      if (re) {
        re.lastIndex = 0;
        if (!re.test(ln)) continue;
      }
      matchedIdx.push(i);
    }
  } else {
    for (let i = 0; i < lastLines.length; i++) matchedIdx.push(i);
  }
  const n = 100;
  const tail = opts.tail ?? true;
  let start: number;
  let end: number;
  if (tail) {
    end = matchedIdx.length;
    start = Math.max(0, end - n);
  } else {
    start = opts.offset ?? 0;
    end = Math.min(matchedIdx.length, start + n);
  }
  const slice = matchedIdx.slice(start, end).map((i) => lastLines[i]);
  return { text: slice.join("\n"), totalLines: matchedIdx.length, nextOffset: end, hasMore: end < matchedIdx.length };
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

let child: { cmd: string; kind: Kind; proc: IPty; start: number } | null = null;
let lastResult: { cmd: string; exit: number | null; end: number } | null = null;
let exitWaiters: (() => void)[] = [];

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
  child = { cmd, kind, proc, start: Date.now() };
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

function doBuild(): void {
  if (child) return;
  launch(specBuild());
}
function doFlash(): void {
  if (child) return;
  launch(specFlash());
}
function doFlashMonitor(): void {
  if (child) return;
  launch(specFlashMonitor());
}
function doBuildFlashMonitor(): void {
  if (child) return;
  launch(specBuildFlashMonitor());
}
function doMonitor(): void {
  if (child) return;
  launch(specMonitor());
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
  { label: "Build", run: doBuild, disabled: () => !!child },
  { label: "Flash", run: doFlash, disabled: () => !!child },
  { label: "Flash+Mon", run: doFlashMonitor, disabled: () => !!child },
  { label: "All", run: doBuildFlashMonitor, disabled: () => !!child },
  { label: "Monitor", run: doMonitor, disabled: () => !!child },
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

// ---------------------------------------------------------------------------
// MCP server
// ---------------------------------------------------------------------------
function createServer(): McpServer {
  const mcp = new McpServer({ name: "idf-mcp", version: "0.7.0" });

  mcp.resource(
    "idf instructions",
    "idf://instructions",
    { description: "Task instructions for the agent driving this ESP-IDF console.", mimeType: "text/plain" },
    async (uri) => ({
      contents: [
        {
          uri: uri.toString(),
          mimeType: "text/plain",
          text:
            "You drive the ESP-IDF project console. The human watches the same TUI and can " +
            "also press buttons (Build/Flash/Flash+Mon/Monitor/Reboot/Stop). Prefer idf_build, " +
            "idf_flash, idf_flash_monitor, idf_monitor for the standard workflow. " +
            "idf_flash_monitor flashes then attaches the monitor with --no-reset so the chip " +
            "reboots only once. Interrupt with idf_interrupt. Reboot the chip with idf_reboot. " +
            "Read output with idf_read_output.",
        },
      ],
    }),
  );

  mcp.tool(
    "idf_execute",
    {
      command: z.string().describe("Arbitrary shell command to run (bash -c) in the IDF project."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for completion in ms (default 600000)."),
      tail: z.number().int().min(1).max(100).describe("How many tail lines of output to return."),
    },
    async ({ command, timeoutMs, tail }) => {
      const busy = busyError();
      if (busy) return busy;
      return runBlocking(specExecute(command), timeoutMs, tail);
    },
  );

  mcp.tool(
    "idf_build",
    {
      extra: z.string().regex(/^[\w\-.= /]+$/, "extra may only contain idf.py build flags").optional().describe("Extra idf.py build arguments."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for completion in ms."),
      tail: z.number().int().min(1).max(100).describe("How many tail lines of output to return."),
    },
    async ({ extra, timeoutMs, tail }) => {
      const busy = busyError();
      if (busy) return busy;
      return runBlocking(specBuild(extra), timeoutMs, tail);
    },
  );

  mcp.tool(
    "idf_flash",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
      timeoutMs: z.number().int().min(1000).max(3_600_000).optional().describe("Max wait for completion in ms."),
      tail: z.number().int().min(1).max(100).describe("How many tail lines of output to return."),
    },
    async ({ port, timeoutMs, tail }) => {
      const busy = busyError();
      if (busy) return busy;
      return runBlocking(specFlash(port), timeoutMs, tail);
    },
  );

  mcp.tool(
    "idf_monitor",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
    },
    async ({ port }) => {
      const busy = busyError();
      if (busy) return busy;
      const spec = specMonitor(port);
      launch(spec);
      return { content: [{ type: "text", text: `started: ${spec.cmd}` }] };
    },
  );

  mcp.tool(
    "idf_flash_monitor",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
    },
    async ({ port }) => {
      const busy = busyError();
      if (busy) return busy;
      const spec = specFlashMonitor(port);
      launch(spec);
      return { content: [{ type: "text", text: `started: ${spec.cmd}` }] };
    },
  );

  mcp.tool(
    "idf_build_flash_monitor",
    {
      port: z.string().regex(/^[A-Za-z0-9_.\-]+$/, "port must be a serial device path").optional().describe("Serial port, e.g. /dev/ttyUSB0."),
    },
    async ({ port }) => {
      const busy = busyError();
      if (busy) return busy;
      const spec = specBuildFlashMonitor(port);
      launch(spec);
      return { content: [{ type: "text", text: `started: ${spec.cmd}` }] };
    },
  );

  mcp.tool(
    "idf_reboot",
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

  mcp.tool("idf_interrupt", {}, async () => {
    const cmd = child?.cmd ?? null;
    interruptCurrent();
    return {
      content: [{ type: "text", text: cmd ? `sent interrupt to: ${cmd}` : "nothing running" }],
    };
  });

  mcp.tool(
    "idf_read_output",
    {
      tail: z.boolean().optional().describe("Return the most recent lines (default true); false reads from offset"),
      offset: z.number().int().min(0).optional().describe("Line index to start from (used when tail=false)"),
      filter: z.string().optional().describe("Regex filter on line content"),
      level: z.enum(["I", "W", "E"]).optional().describe("Filter by ESP-IDF log level prefix"),
      clear: z.boolean().optional().describe("Clear the buffer after reading (default false)"),
    },
    async ({ tail, offset, filter, level, clear }) => {
      const r = readOutput({ tail, offset, filter, level });
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

  mcp.tool("idf_log_stats", {}, async () => {
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

  mcp.tool("idf_status", {}, async () => {
    return {
      content: [
        {
          type: "text",
          text: JSON.stringify(
            {
              projectDir,
              mock: MOCK,
              running: child ? { cmd: child.cmd, kind: child.kind, seconds: Math.round((Date.now() - child.start) / 1000) } : null,
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
