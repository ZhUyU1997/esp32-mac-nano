#!/usr/bin/env node
// idf-mcp: ESP-IDF project console driven by an MCP agent, watched by a human.
//
// The agent fully controls the session (flash / monitor / build / any command)
// and can interrupt. The user only watches: pty output streams to the terminal
// in real time; the user does not type (Ctrl-C still works as an emergency
// brake: idle -> quit, running -> interrupts the agent's command).
//
// Output model: only the output of the most recent command is retained
// (`lastLines`). While a command is running the buffer grows live; when it
// completes the exit code is recorded. Reading never mixes commands.
//
// MCP server: Streamable HTTP on http://127.0.0.1:8765/mcp
// Mock mode (no real hardware / no real idf.py): MCP_IDF_MOCK=1
import pty from "node-pty";
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StreamableHTTPServerTransport } from "@modelcontextprotocol/sdk/server/streamableHttp.js";
import { z } from "zod";
import express, { type Request, type Response } from "express";
import http from "node:http";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { execSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { randomUUID } from "node:crypto";

const PORT = Number(process.env.MCP_IDF_PORT ?? 8765);
const HOST = process.env.MCP_IDF_HOST ?? "127.0.0.1";
const RING_MAX = Number(process.env.MCP_IDF_BUFFER ?? 16 * 1024 * 1024); // 16 MiB
const MAX_LINE = 4096; // per-line cap (bytes); longer lines are truncated
const MOCK = process.env.MCP_IDF_MOCK === "1";
const MOCK_FLASH = path.join(path.dirname(fileURLToPath(import.meta.url)), "mock", "mock-flash.mjs");
const MOCK_MONITOR = path.join(path.dirname(fileURLToPath(import.meta.url)), "mock", "mock-monitor.mjs");

// IDF environment activation:
//   --activate <script>  use this activation script
//   (omitted)            probe the default ESP-IDF install path
//   --activate ""        disable probing entirely
const ACTIVATE_DEFAULT = path.join(os.homedir(), ".espressif", "tools", "activate_idf_v5.5.4.sh");
let activateScript: string | null = null;
{
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--activate" && i + 1 < argv.length) {
      activateScript = argv[i + 1];
    }
  }
  if (activateScript === null) {
    // no flag: use the default path if it exists
    if (fs.existsSync(ACTIVATE_DEFAULT)) activateScript = ACTIVATE_DEFAULT;
  }
}

// Always run bash with a generated rcfile so we can brand the prompt (so the
// watcher can tell this terminal apart from a normal one) and re-assert the
// completion sentinel after the user's .bashrc runs.
// The prompt is dynamic: node writes the agent's state to a status file, and
// the rcfile's PROMPT_COMMAND reads it to show [idle] or the running command.
const rcPath = path.join(os.tmpdir(), `idf-mcp-rc-${process.pid}.sh`);
const statePath = path.join(os.tmpdir(), `idf-mcp-state-${process.pid}`);
{
  const parts: string[] = [];
  if (activateScript) parts.push(`source "${activateScript}"`);
  parts.push(`[ -f "$HOME/.bashrc" ] && source "$HOME/.bashrc"`);
  // Dynamic prompt after .bashrc (WSL defaults to \u@\h:\w): brand + working
  // dir + agent state. No trailing $, the terminal is watch-only.
  parts.push(`__idf_mcp_state=${JSON.stringify(statePath)}`);
  parts.push(`__idf_mcp_prompt() {`);
  parts.push(`  local rc=$?`);
  parts.push(`  # completion sentinel first, then wait briefly so node can flip the`);
  parts.push(`  # state file to IDLE before we render the prompt (bash renders the`);
  parts.push(`  # prompt only after the command finished, so it would otherwise`);
  parts.push(`  # always show the just-finished command).`);
  parts.push(`  echo -e "\\x1b]0;MCP_CMD_END:$rc\\x07"`);
  parts.push(`  sleep 0.03`);
  parts.push(`  local st=IDLE`);
  parts.push(`  [ -f "$__idf_mcp_state" ] && st=$(cat "$__idf_mcp_state")`);
  parts.push(`  local p="\\[\\e[38;5;208m\\][idf-mcp]\\[\\e[0m\\] \\w"`);
  parts.push(`  case "$st" in`);
  parts.push(`    RUNNING:*) p="$p \\[\\e[33m\\]▶ \${st#RUNNING:}\\[\\e[0m\\]";;`);
  parts.push(`    *) p="$p \\[\\e[32m\\][idle]\\[\\e[0m\\]";;`);
  parts.push(`  esac`);
  parts.push(`  PS1="$p "`);
  parts.push(`}`);
  parts.push(`PROMPT_COMMAND='__idf_mcp_prompt'`);
  fs.writeFileSync(rcPath, parts.join("\n") + "\n");
}

function writeState(st: string): void {
  try {
    fs.writeFileSync(statePath, st);
  } catch {
    /* tmpfs full etc: prompt just stays stale, harmless */
  }
}

// ---------------------------------------------------------------------------
// output of the most recent command, line-oriented, ANSI-stripped.
// The screen still shows raw data (colors preserved).
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
    ln = capLine(ln.replace(ANSI_RE, "").replace(/^\r+/, ""));
    lastLines.push(ln);
    lastBytes += Buffer.byteLength(ln);
    while (lastBytes > RING_MAX && lastLines.length > 1) {
      lastBytes -= Buffer.byteLength(lastLines.shift()!);
    }
  }
}

function drainPending(): void {
  if (!pending) return;
  const ln = capLine(pending.replace(ANSI_RE, "").replace(/^\r+/, ""));
  pending = "";
  lastLines.push(ln);
  lastBytes += Buffer.byteLength(ln);
  while (lastBytes > RING_MAX && lastLines.length > 1) {
    lastBytes -= Buffer.byteLength(lastLines.shift()!);
  }
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
  const n = 100; // fixed page size; the agent paginates via offset
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
// spawn the shared shell (bash) in a pty, inside the IDF project directory
// ---------------------------------------------------------------------------
const projectDir = process.env.MCP_IDF_PROJECT || process.cwd();
const shell = pty.spawn(process.env.SHELL || "bash", ["--rcfile", rcPath], {
    name: "xterm-256color",
    cols: process.stdout.columns || 80,
    rows: process.stdout.rows || 24,
    cwd: projectDir,
    env: {
      ...process.env,
      // Emit a completion sentinel (OSC title, invisible on screen) every time
      // the prompt returns — works even when Ctrl-C aborts the command line.
      PROMPT_COMMAND: 'echo -e "\\x1b]0;MCP_CMD_END:$?\\x07"',
    },
  },
);

// user terminal: echo off (user does not type), restore on exit
const isTTY = Boolean(process.stdin.isTTY && process.stdout.isTTY);
let savedStty: string | null = null;
if (isTTY) {
  try {
    savedStty = execSync("stty -g", { stdio: ["inherit", "pipe", "inherit"] }).toString().trim();
    execSync("stty -echo", { stdio: ["inherit", "inherit", "inherit"] });
  } catch {
    /* not a tty */
  }
}
// NOTE: stdin is intentionally NOT forwarded — the user cannot type.
// Ctrl-C still arrives as SIGINT because ISIG stays enabled (normal mode).

// Sentinel is the full OSC title sequence \x1b]0;MCP_CMD_END:<exit>\x07, so
// plain text output containing "MCP_CMD_END" cannot collide. A small tail
// buffer handles the sequence arriving split across onData chunks.
const OSC_CMD_END_RE = /\x1b\]0;MCP_CMD_END:(-?\d+)\x07/g;
let oscTail = "";
function setTitle(t: string): void {
  process.stdout.write(`\x1b]0;${t}\x07`);
}

let running: { cmd: string; start: number } | null = null;
let lastCmd: { cmd: string; exit: number | null; end: number } | null = null;
// waiters for blocking tool calls (resolved when the sentinel fires)
let doneWaiters: (() => void)[] = [];

function waitForDone(timeoutMs: number): Promise<boolean> {
  return new Promise((resolve) => {
    if (!running) {
      resolve(true);
      return;
    }
    let settled = false;
    const finish = (ok: boolean): void => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      const i = doneWaiters.indexOf(waiter);
      if (i >= 0) doneWaiters.splice(i, 1);
      resolve(ok);
    };
    const timer = setTimeout(() => finish(false), timeoutMs);
    const waiter = (): void => finish(true);
    doneWaiters.push(waiter);
  });
}

shell.onData((data: string) => {
  if (isTTY) process.stdout.write(data);
  pushOutput(data);
  // sentinel detection after pushOutput so the same chunk's lines are counted
  oscTail = (oscTail + data).slice(-256);
  let m: RegExpExecArray | null;
  OSC_CMD_END_RE.lastIndex = 0;
  while ((m = OSC_CMD_END_RE.exec(oscTail)) !== null) {
    const exit = Number(m[1]);
    if (running) {
      drainPending();
      lastExit = exit;
      lastCmd = { cmd: running.cmd, exit, end: Date.now() };
      writeState("IDLE");
      console.error(`[idf-mcp] done: ${running.cmd} (exit ${exit})`);
      running = null;
      const ws = doneWaiters;
      doneWaiters = [];
      for (const w of ws) w();
    }
    setTitle(`idf-mcp: ${projectDir}`);
    oscTail = ""; // sentinel is transient; consume it
    break;
  }
});

// window resize -> pty
process.stdout.on("resize", () => {
  try {
    shell.resize(process.stdout.columns, process.stdout.rows);
  } catch {
    /* pty may be gone */
  }
});

// ---------------------------------------------------------------------------
// command injection. A new command starts a fresh output buffer, so reads
// never mix commands; long-running output (monitor) grows the buffer live.
// ---------------------------------------------------------------------------
function inject(cmd: string): void {
  drainPending();
  lastLines = [];
  lastBytes = 0;
  lastCmdName = cmd;
  lastExit = null;
  running = { cmd, start: Date.now() };
  writeState(`RUNNING:${cmd}`);
  setTitle(`\u25b6 ${cmd}`);
  console.error(`[idf-mcp] agent: ${cmd}`);
  shell.write(cmd + "\r");
}

// Reject a new injection while a command is still running.
function busyError(): { content: { type: "text"; text: string }[]; isError: true } | null {
  if (!running) return null;
  return {
    content: [
      {
        type: "text",
        text: `a command is already running: ${running.cmd} — interrupt it first (idf_interrupt) or wait for completion`,
      },
    ],
    isError: true,
  };
}

function mockCommand(sub: string): string {
  if (sub === "flash") return `node "${MOCK_FLASH}"`;
  if (sub === "monitor") return `node "${MOCK_MONITOR}"`;
  return `idf.py ${sub}`;
}

function idfCommand(sub: string, port?: string): string {
  if (MOCK) return mockCommand(sub); // mock ignores port: no serial device
  return `idf.py ${sub}${port ? ` -p ${port}` : ""}`;
}

// ---------------------------------------------------------------------------
// blocking execution for non-log commands: inject, then wait for the
// completion sentinel. Returns a tail of the output; the agent can read the
// full output via idf_read_output if needed. Times out to avoid hanging the
// tool call forever (e.g. a command that never exits).
// ---------------------------------------------------------------------------
const DEFAULT_TIMEOUT_MS = 600_000; // 10 min
const TAIL_LINES = 10;

async function runBlocking(
  cmd: string,
  timeoutMs?: number,
  tailLines: number = TAIL_LINES,
): Promise<{ content: { type: "text"; text: string }[] }> {
  inject(cmd);
  const done = await waitForDone(timeoutMs ?? DEFAULT_TIMEOUT_MS);
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
// exit / signals
// ---------------------------------------------------------------------------
let exiting = false;
function cleanup(): void {
  if (exiting) return;
  exiting = true;
  if (savedStty) {
    try {
      execSync(`stty ${savedStty}`, { stdio: ["inherit", "inherit", "inherit"] });
    } catch {
      /* ignore */
    }
  }
  process.stdout.write("\r\n[idf-mcp] session ended\r\n");
  if (rcPath) {
    try {
      fs.unlinkSync(rcPath);
    } catch {
      /* ignore */
    }
  }
  try {
    fs.unlinkSync(statePath);
  } catch {
    /* ignore */
  }
  process.exit(0);
}
shell.onExit(() => cleanup());
process.on("SIGTERM", () => {
  try {
    shell.kill();
  } catch {
    /* already gone */
  }
});

// ---------------------------------------------------------------------------
// interrupt the running command. idf.py monitor runs the terminal in raw mode
// (ISIG off), so Ctrl-C is *not* a signal there: esp-idf-monitor forwards it
// to the chip as a serial interrupt and only exits on its exit key Ctrl-]
// (0x1d). Every other command gets plain Ctrl-C (0x03 = SIGINT to the
// foreground process group).
// ---------------------------------------------------------------------------
function interruptRunning(): void {
  if (!running) return;
  const isMonitor = /monitor/.test(running.cmd);
  const key = isMonitor ? "\x1d" : "\x03";
  console.error(`[idf-mcp] interrupt: ${isMonitor ? "Ctrl-] (monitor exit key)" : "Ctrl-C"} -> ${running.cmd}`);
  shell.write(key);
}

// User Ctrl-C: emergency brake. The pty bash lives in its own process group,
// so the user's SIGINT only reaches us: forward the right key into the pty
// when a command is running (same as idf_interrupt), quit when idle.
process.on("SIGINT", () => {
  if (running) {
    console.error("[idf-mcp] user brake: forwarding interrupt to the running command");
    interruptRunning();
  } else {
    cleanup();
  }
});

// ---------------------------------------------------------------------------
// MCP tools. One McpServer instance per client connection (SDK limitation).
// ---------------------------------------------------------------------------
function createServer(): McpServer {
  const mcp = new McpServer({ name: "idf-mcp", version: "0.1.0" });

  mcp.resource(
    "idf instructions",
    "idf://instructions",
    {
      description: "Task instructions for the agent driving this ESP-IDF console.",
      mimeType: "text/plain",
    },
    async (uri) => ({
      contents: [
        {
          uri: uri.toString(),
          mimeType: "text/plain",
          text:
            "You drive the ESP-IDF project console. The human user only watches; " +
            "you are the one executing commands. Prefer idf_build, idf_flash, " +
            "idf_monitor for the standard workflow. Interrupt long-running commands " +
            "with idf_interrupt. Read output with idf_read_output.",
        },
      ],
    }),
  );

  mcp.tool(
    "idf_execute",
    {
      command: z.string().describe("Arbitrary shell command to run in the IDF project shell."),
      timeoutMs: z
        .number()
        .int()
        .min(1000)
        .max(3_600_000)
        .optional()
        .describe("Max wait for completion in ms (default 600000). On timeout the command keeps running; read idf_status / idf_read_output or interrupt."),
      tail: z
        .number()
        .int()
        .min(1)
        .max(100)
        .describe("How many tail lines of output to return."),
    },
    async ({ command, timeoutMs, tail }) => {
      const busy = busyError();
      if (busy) return busy;
      return runBlocking(command, timeoutMs, tail);
    },
  );

  mcp.tool(
    "idf_build",
    {
      extra: z
        .string()
        .regex(/^[\w\-.= /]+$/, "extra may only contain idf.py build flags (no newlines)")
        .optional()
        .describe("Extra idf.py build arguments, e.g. '--no-ccache'."),
      timeoutMs: z
        .number()
        .int()
        .min(1000)
        .max(3_600_000)
        .optional()
        .describe("Max wait for completion in ms (default 600000). On timeout the build keeps running."),
      tail: z
        .number()
        .int()
        .min(1)
        .max(100)
        .describe("How many tail lines of output to return."),
    },
    async ({ extra, timeoutMs, tail }) => {
      const busy = busyError();
      if (busy) return busy;
      return runBlocking(idfCommand(`build${extra ? ` ${extra}` : ""}`), timeoutMs, tail);
    },
  );

  mcp.tool(
    "idf_flash",
    {
      port: z
        .string()
        .regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path")
        .optional()
        .describe("Serial port, e.g. /dev/ttyUSB0."),
      timeoutMs: z
        .number()
        .int()
        .min(1000)
        .max(3_600_000)
        .optional()
        .describe("Max wait for completion in ms (default 600000). On timeout the flash keeps running."),
      tail: z
        .number()
        .int()
        .min(1)
        .max(100)
        .describe("How many tail lines of output to return."),
    },
    async ({ port, timeoutMs, tail }) => {
      const busy = busyError();
      if (busy) return busy;
      return runBlocking(idfCommand("flash", port), timeoutMs, tail);
    },
  );

  mcp.tool(
    "idf_monitor",
    {
      port: z
        .string()
        .regex(/^[A-Za-z0-9_.\-/]+$/, "port must be a serial device path")
        .optional()
        .describe("Serial port, e.g. /dev/ttyUSB0."),
    },
    async ({ port }) => {
      const busy = busyError();
      if (busy) return busy;
      // monitor is a long-running log stream: fire-and-forget, the agent
      // reads live output and stops it with idf_interrupt.
      const cmd = idfCommand("monitor", port);
      inject(cmd);
      return { content: [{ type: "text", text: `started: ${cmd}` }] };
    },
  );

  mcp.tool(
    "idf_interrupt",
    {},
    async () => {
      const cmd = running?.cmd ?? null;
      const isMonitor = cmd ? /monitor/.test(cmd) : false;
      interruptRunning();
      return {
        content: [
          {
            type: "text",
            text: cmd
              ? `sent ${isMonitor ? "Ctrl-] (monitor exit key)" : "Ctrl-C"} to: ${cmd}`
              : "sent Ctrl-C (nothing running)",
          },
        ],
      };
    },
  );

  mcp.tool(
    "idf_read_output",
    {
      tail: z
        .boolean()
        .optional()
        .describe("Return the most recent lines (default true); false reads from offset"),
      offset: z
        .number()
        .int()
        .min(0)
        .optional()
        .describe("Line index to start from (used when tail=false)"),
      filter: z
        .string()
        .optional()
        .describe("Regex filter on line content (invalid regex falls back to literal substring)"),
      level: z
        .enum(["I", "W", "E"])
        .optional()
        .describe("Filter by ESP-IDF log level prefix (I=info, W=warn, E=error)"),
      clear: z
        .boolean()
        .optional()
        .describe("Clear the buffer after reading (default false)"),
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
              running: running?.cmd ?? null,
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

  mcp.tool(
    "idf_log_stats",
    {},
    async () => {
      drainPending();
      let info = 0;
      let warn = 0;
      let error = 0;
      let other = 0;
      for (const ln of lastLines) {
        const m = IDF_LEVEL_RE.exec(ln);
        if (!m) {
          other++;
        } else if (m[1] === "I") {
          info++;
        } else if (m[1] === "W") {
          warn++;
        } else {
          error++;
        }
      }
      return {
        content: [
          {
            type: "text",
            text: JSON.stringify(
              {
                command: lastCmdName,
                exit: lastExit,
                running: running?.cmd ?? null,
                totalLines: lastLines.length,
                bytes: lastBytes,
                levels: { info, warn, error, other },
              },
              null,
              2,
            ),
          },
        ],
      };
    },
  );

  mcp.tool(
    "idf_status",
    {},
    async () => {
      let cwd: string | null = null;
      try {
        cwd = fs.readlinkSync(`/proc/${shell.pid}/cwd`);
      } catch {
        /* not linux / gone */
      }
      return {
        content: [
          {
            type: "text",
            text: JSON.stringify(
              {
                projectDir,
                shellCwd: cwd,
                mock: MOCK,
                running: running ? { cmd: running.cmd, seconds: Math.round((Date.now() - running.start) / 1000) } : null,
                lastCmd: lastCmd
                  ? { cmd: lastCmd.cmd, exit: lastCmd.exit, secondsAgo: Math.round((Date.now() - lastCmd.end) / 1000) }
                  : null,
                bufferBytes: lastBytes,
                bufferLines: lastLines.length,
              },
              null,
              2,
            ),
          },
        ],
      };
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

http.createServer(app).listen(PORT, HOST, () => {
  console.error(`[idf-mcp] MCP server: http://${HOST}:${PORT}/mcp`);
  console.error(`[idf-mcp] project: ${projectDir}  mock: ${MOCK}  activate: ${activateScript ?? "(none)"}`);
  console.error(`[idf-mcp] agent drives the console; user watches (Ctrl-C quits when idle)`);
});
