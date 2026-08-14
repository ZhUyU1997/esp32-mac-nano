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
const rb = await call("idf_reboot");
console.log("[reboot via monitor]", typeof rb === "string" && rb.includes("via monitor"));
await sleep(1200);
out = await call("idf_read_output", { tail: true });
console.log("[reboot] output has chip reset:", out.text.includes("chip reset (mock)"));
await call("idf_interrupt");
await sleep(600);
let st = JSON.parse(await call("idf_status"));
console.log("[monitor interrupted] running:", st.running, "| exit:", st.lastCmd?.exit);

// --- flash_monitor: async (flash + monitor in one, mock) ---
const fm = await call("idf_flash_monitor");
console.log("[flash_monitor] started:", typeof fm === "string" && fm.includes("started"));
await sleep(2500);
out = await call("idf_read_output", { tail: true });
console.log("[flash_monitor] has flash OK:", out.text.includes("flash OK (mock)"), "| has log:", out.text.includes("mock log line"));
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

await client.close();
console.log("TEST DONE");
