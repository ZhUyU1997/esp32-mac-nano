// Test client for idf-mcp. Mock workflow:
// flash (blocking) -> monitor (async) -> interrupt -> execute timeout -> pagination.
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StreamableHTTPClientTransport } from "@modelcontextprotocol/sdk/client/streamableHttp.js";

const url = process.argv[2] || "http://127.0.0.1:8765/mcp";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const transport = new StreamableHTTPClientTransport(new URL(url));
const client = new Client({ name: "idf-mcp-test", version: "0.0.1" });
await client.connect(transport);

const tools = await client.listTools();
console.log("TOOLS:", tools.tools.map((t) => t.name).join(", "));

const call = async (name, args = {}) => {
  const r = await client.callTool({ name, arguments: args });
  const texts = r.content.map((c) => c.text);
  return texts.length === 1 ? texts[0] : { text: texts[0], meta: JSON.parse(texts[1]) };
};

// --- flash: BLOCKING, returns tail + status when done ---
const flash = await call("idf_flash", { tail: 10 });
console.log("[flash] status:", flash.meta.status, "| exit:", flash.meta.exit, "| tail has OK:", flash.text.includes("flash OK (mock)"), "| totalLines:", flash.meta.totalLines);

// --- monitor: async (fire-and-forget), read live, interrupt ---
const mon = await call("idf_monitor");
console.log("[monitor] async started:", typeof mon === "string" && mon.includes("started"));
await sleep(3000);
let out = await call("idf_read_output", { level: "E", tail: false });
console.log("[monitor live] all E lines:", out.text.split("\n").every((l) => /^E \(\d+\)/.test(l)), "| count:", out.text.split("\n").length);
const stats = JSON.parse(await call("idf_log_stats"));
console.log("[stats]", JSON.stringify(stats.levels), "| running:", stats.running ? "yes" : "no");
await call("idf_interrupt");
await sleep(800);
let st = JSON.parse(await call("idf_status"));
console.log("[monitor interrupted] running:", st.running, "| exit:", st.lastCmd?.exit);

// --- execute with short timeout: returns running, then interrupt ---
const ex = await call("idf_execute", { command: "sleep 30", timeoutMs: 1000, tail: 10 });
console.log("[execute timeout] status:", ex.meta.status, "| timedOut:", ex.meta.timedOut);
await call("idf_interrupt");
await sleep(800);
st = JSON.parse(await call("idf_status"));
console.log("[after interrupt] running:", st.running, "| exit:", st.lastCmd?.exit, "(expect 130)");

// --- pagination on a completed command ---
await call("idf_execute", { command: "seq 1 200", tail: 10 });
const page1 = await call("idf_read_output", { tail: false, offset: 0, filter: "^\\d+$" });
const nums = page1.text.split("\n").map(Number);
const page2 = await call("idf_read_output", { tail: false, offset: page1.meta.nextOffset, filter: "^\\d+$" });
const nums2 = page2.text.split("\n").map(Number);
console.log("[pagination] p1:", nums[0], "-", nums[nums.length - 1], "| p2:", nums2[0], "-", nums2[nums2.length - 1], "| hasMore:", page2.meta.hasMore);

await client.close();
console.log("TEST DONE");
