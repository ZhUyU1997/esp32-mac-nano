// Test client for idf-mcp. Mock workflow:
// flash (blocking) -> monitor (async) -> reboot (via monitor) -> interrupt
// -> flash_monitor (async) -> interrupt -> execute timeout -> reboot idle.
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StreamableHTTPClientTransport } from "@modelcontextprotocol/sdk/client/streamableHttp.js";

const url = process.argv[2] || "http://127.0.0.1:8765/mcp";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const transport = new StreamableHTTPClientTransport(new URL(url));
const client = new Client({ name: "idf-mcp-test", version: "0.0.1" });
await client.connect(transport);

const tools = await client.listTools();
const names = tools.tools.map((t) => t.name);
console.log("TOOLS:", names.join(", "));
for (const required of ["idf_flash_monitor", "idf_reboot"]) {
  console.log(`[tools] has ${required}:`, names.includes(required));
}

const call = async (name, args = {}) => {
  const r = await client.callTool({ name, arguments: args });
  const texts = r.content.map((c) => c.text);
  return texts.length === 1 ? texts[0] : { text: texts[0], meta: JSON.parse(texts[1]) };
};

// --- flash: BLOCKING ---
const flash = await call("idf_flash", { tail: 10 });
console.log("[flash] status:", flash.meta.status, "| exit:", flash.meta.exit, "| tail has OK:", flash.text.includes("flash OK (mock)"));

// --- monitor: async, then reboot via monitor ---
await call("idf_monitor");
await sleep(2500);
let out = await call("idf_read_output", { tail: true });
console.log("[monitor live] has mock log:", out.text.includes("mock log line"));

// --- idf_read_output grep-style options (mock logs: "I/W/E (n) app_main: mock log line N") ---
let g = await call("idf_read_output", { level: "E", count: true });
console.log("[read_output -c] count > 0:", g.meta.totalLines > 0, "|", g.text);
g = await call("idf_read_output", { filter: "APP_MAIN", caseSensitive: false, tail: false });
console.log("[read_output -i] matches lowercase:", g.meta.totalLines > 0);
g = await call("idf_read_output", { filter: "APP_MAIN", tail: false });
console.log("[read_output case] sensitive no match:", g.meta.totalLines === 0);
g = await call("idf_read_output", { filter: "app_main", invert: true, tail: false });
console.log("[read_output -v] invert leaves no app_main:", !g.text.includes("app_main"));
g = await call("idf_read_output", { level: "E", context: 1, tail: false });
console.log("[read_output -C] context includes neighbors:", g.text.split("\n").some((l) => l.startsWith("W ") || l.startsWith("I ")));
const rb = await call("idf_reboot");
console.log("[reboot via monitor]", typeof rb === "string" && rb.includes("via monitor"));
await sleep(1200);
out = await call("idf_read_output", { tail: true });
console.log("[reboot] output has chip reset:", out.text.includes("chip reset (mock)"));
await call("idf_interrupt");
await sleep(600);
let st = JSON.parse(await call("idf_status"));
console.log("[monitor interrupted] running:", st.running, "| exit:", st.lastCmd?.exit);

// --- flash_monitor: blocks until the monitor is attached (flash done),
//     wait= keeps blocking until the app logs ---
const fm = await call("idf_flash_monitor", { wait: "mock log line", timeoutMs: 15000 });
console.log("[flash_monitor wait] status:", fm.meta.status, "| matched:", fm.meta.matched, "| running:", fm.meta.running);
// default (no wait): blocks until the monitor is attached (flash done) — and
// auto-stops the previously attached monitor first
const fm2 = await call("idf_flash_monitor", { timeoutMs: 15000 });
console.log("[flash_monitor default] status:", fm2.meta.status, "| matched:", fm2.meta.matched, "| running:", fm2.meta.running);
// wait for the firmware to start printing (returns just the matched line)
const wf = await call("idf_wait_for", { pattern: "mock log line", timeoutMs: 5000 });
console.log("[wait_for] matched line:", wf);
// forward-only: a pattern that only appeared in history must NOT match again
const fw = await call("idf_wait_for", { pattern: "Hard resetting via RTS pin", timeoutMs: 1500 });
console.log("[wait_for forward-only] status:", fw, "(expect timedOut)");
// includePast: the same historical pattern MUST match
const hp = await call("idf_wait_for", { pattern: "Hard resetting via RTS pin", includePast: true, timeoutMs: 2000 });
console.log("[wait_for includePast] line:", hp);
await call("idf_interrupt");
await sleep(600);
st = JSON.parse(await call("idf_status"));
console.log("[flash_monitor interrupted] running:", st.running);

// --- execute with short timeout ---
const ex = await call("idf_execute", { command: "sleep 30", timeoutMs: 1000, tail: 10 });
console.log("[execute timeout] status:", ex.meta.status, "| timedOut:", ex.meta.timedOut);
await call("idf_interrupt");
await sleep(600);
st = JSON.parse(await call("idf_status"));
console.log("[after interrupt] running:", st.running, "| exit:", st.lastCmd?.exit, "(expect 130)");

// --- reboot when idle: starts monitor (which resets on startup) ---
const rb2 = await call("idf_reboot");
console.log("[reboot idle] starts monitor:", typeof rb2 === "string" && rb2.includes("started monitor"));
await sleep(1200);
await call("idf_interrupt");
await sleep(600);
st = JSON.parse(await call("idf_status"));
console.log("[after idle reboot] running:", st.running, "| lastCmd:", st.lastCmd?.cmd);

// --- wait without timeoutMs must be rejected (no launch) ---
const noT = await call("idf_flash_monitor", { wait: "Hard resetting via RTS pin" });
console.log("[wait no timeout] rejected:", typeof noT === "string" && noT.includes("timeoutMs is required"));
st = JSON.parse(await call("idf_status"));
console.log("[wait no timeout] still idle:", st.running === null);

await client.close();
console.log("TEST DONE");
