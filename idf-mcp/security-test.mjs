// Security / robustness regression tests for idf-mcp (mock mode):
// port/extra injection, concurrent injection, sentinel collision, long lines.
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StreamableHTTPClientTransport } from "@modelcontextprotocol/sdk/client/streamableHttp.js";

const url = process.argv[2] || "http://127.0.0.1:8765/mcp";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const transport = new StreamableHTTPClientTransport(new URL(url));
const client = new Client({ name: "idf-mcp-sec", version: "0.0.1" });
await client.connect(transport);
const call = async (name, args = {}) => {
  const r = await client.callTool({ name, arguments: args });
  return { isErr: Boolean(r.isError), texts: r.content.map((c) => c.text) };
};

// 1. port shell injection rejected by zod
let r = await call("idf_flash", { port: "/dev/ttyUSB0; echo PWNED", tail: 10 });
console.log("[port injection] rejected:", r.isErr, "|", r.texts[0].slice(0, 70));

// 2. extra injection rejected
r = await call("idf_build", { extra: "--no-ccache; rm -rf /", tail: 10 });
console.log("[extra injection] rejected:", r.isErr, "|", r.texts[0].slice(0, 70));

// 2b. newline injection in extra rejected (\n would break out of the arg line)
r = await call("idf_build", { extra: "--no-ccache\nrm -rf /", tail: 10 });
console.log("[extra newline] rejected:", r.isErr, "|", r.texts[0].slice(0, 70));

// 3. legitimate port passes schema, command completes
r = await call("idf_flash", { port: "/dev/ttyUSB0", tail: 10 });
console.log("[legit port] accepted:", !r.isErr);
await sleep(3000);

// 4. concurrent injection rejected while a command is running
//    (short timeoutMs so the call returns while `sleep 30` is still running)
r = await call("idf_execute", { command: "sleep 30", timeoutMs: 2000, tail: 10 });
console.log("[concurrent] first accepted:", !r.isErr);
r = await call("idf_execute", { command: "echo should-not-run", tail: 10 });
console.log("[concurrent] second rejected:", r.isErr, "|", r.texts[0].slice(0, 60));
await call("idf_interrupt");
await sleep(800);

// 5. sentinel collision: plain text "MCP_CMD_END:5" must not corrupt state
r = await call("idf_execute", { command: "echo fake-MCP_CMD_END:5; seq 1 3", tail: 10 });
await sleep(1500);
let st = JSON.parse((await call("idf_status")).texts[0]);
console.log(
  "[sentinel collision] running:", st.running,
  "| lastCmd:", st.lastCmd?.cmd, "exit", st.lastCmd?.exit,
  "(expect exit 0 from the real completion, not 5)",
);

// 6. long single line truncated
r = await call("idf_execute", { command: "head -c 10000 /dev/zero | tr '\\0' 'a'", tail: 10 });
await sleep(1500);
const out = await call("idf_read_output", { tail: false, filter: "aaaa" });
const longLines = out.texts[0].split("\n").filter((l) => l.includes("aaaa"));
const maxLen = longLines.length ? Math.max(...longLines.map((l) => l.length)) : 0;
console.log("[long line] max len:", maxLen, "| truncated marker:", longLines.some((l) => l.includes("[truncated]")));

await client.close();
console.log("SEC TEST DONE");
