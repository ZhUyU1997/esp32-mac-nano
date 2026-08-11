import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StreamableHTTPClientTransport } from "@modelcontextprotocol/sdk/client/streamableHttp.js";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const transport = new StreamableHTTPClientTransport(new URL("http://127.0.0.1:8765/mcp"));
const client = new Client({ name: "big", version: "0.0.1" });
await client.connect(transport);
// generate ~1.5 MiB of output
await client.callTool({ name: "idf_execute", arguments: { command: "seq 1 30000 | head -c 1500000 | grep -n . | cut -c1-60" } });
await sleep(3000);
const out = await client.callTool({ name: "idf_read_output", arguments: {} });
const t = out.content[0].text;
console.log("ring buffer length:", t.length, "bytes");
console.log("starts with:", JSON.stringify(t.slice(0, 60)));
console.log("ends with  :", JSON.stringify(t.slice(-60)));
await client.close();
