// Vite config: dev = mock serve (API/WS middleware), build = single-file index.html (vite-plugin-singlefile)
import { defineConfig, type Plugin } from 'vite';
import preact from '@preact/preset-vite';
import { viteSingleFile } from 'vite-plugin-singlefile';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { PNG } from 'pngjs';
import { WebSocketServer, type WebSocket } from 'ws';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// Screenshot frame: real image (640x480 RGBA PNG) → grayscale threshold → 1-bit (1=black 0=white) — same as original server.ts
const FRAME = Buffer.alloc((640 * 480) / 8);
{
  const png = PNG.sync.read(readFileSync(path.join(__dirname, 'test/fixtures/1785865268148.png')));
  for (let y = 0; y < 480; y++) {
    for (let x = 0; x < 640; x++) {
      const i = (y * 640 + x) * 4;
      const gray = (png.data[i] + png.data[i + 1] + png.data[i + 2]) / 3;
      if (gray < 128) FRAME[(y * 640 + x) >> 3] |= 0x80 >> (x & 7);
    }
  }
  console.log('[mock] screenshot: real image → 1-bit frame');
}

const json = (res: any, obj: unknown) => {
  res.writeHead(200, { 'content-type': 'application/json' });
  res.end(JSON.stringify(obj));
};

/* ── WiFi provisioning mock ──
 * /api/wifi/scan: 交替 scanning → AP 列表（每次刷新先转一圈）
 * /api/wifi/config: 提交后 3s 推 CONNECTING，再 3s 推 CONNECTED；密码 "123" 模拟失败回 PROVISIONING */
const WIFI = { PROVISIONING: 1, CONNECTING: 2, CONNECTED: 3 } as const;
const AP_LIST = [
  { ssid: '邻居家WiFi', rssi: -62, auth: 4 },
  { ssid: 'cafe_free', rssi: -77, auth: 0 },
  { ssid: 'Test-2.4G', rssi: -58, auth: 3 },
];
let wifiState: number = WIFI.PROVISIONING;
let failReason = 0; /* 0=无 1=密码错误 2=找不到网络 3=其他 */
let scanDone = false;
const wsClients = new Set<WebSocket>();
function broadcastWifi(st: number, reason = 0) {
  wifiState = st;
  failReason = reason;
  const buf = Buffer.from([0x09, st, reason]);
  for (const ws of wsClients) ws.send(buf);
}

// mock API middleware + WebSocket (logic from original server.ts)
function mockApi(): Plugin {
  const api = (req: any, res: any, next: () => void) => {
    const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
    const p = url.pathname;
    if (p === '/api/status') return json(res, { floppy: false, state: 'CONNECTED', sta: { ssid: 'Test-2.4G', ip: '192.168.1.42' } });
    if (p === '/api/screenshot') {
      res.writeHead(200, { 'content-type': 'application/octet-stream' });
      return res.end(FRAME);
    }
    if (p === '/api/floppy' && req.method === 'POST') {
      let n = 0;
      req.on('data', (c: Buffer) => (n += c.length));
      req.on('end', () => {
        console.log(`[mock] POST /api/floppy ${n} bytes`);
        json(res, { ok: true });
      });
      return;
    }
    if (p === '/api/wifi/scan') {
      if (!scanDone) { scanDone = true; return json(res, { scanning: true }); }
      scanDone = false;
      return json(res, AP_LIST);
    }
    if (p === '/api/wifi/reset') {
      /* dev 工具：重置回配网状态（重启 dev server 或手动调用） */
      wifiState = WIFI.PROVISIONING;
      failReason = 0;
      scanDone = false;
      return json(res, { ok: true });
    }
    if (p === '/api/wifi/done' && req.method === 'POST') {
      /* 成功页“复制并关闭配网热点”：mock 无真实热点，仅回 ok */
      console.log('[mock] POST /api/wifi/done');
      return json(res, { ok: true });
    }
    if (p === '/api/wifi/config' && req.method === 'POST') {
      let body = '';
      req.on('data', (c: Buffer) => (body += c));
      req.on('end', () => {
        let ssid = '', pass = '';
        try { ({ ssid, pass } = JSON.parse(body)); } catch {}
        console.log(`[mock] POST /api/wifi/config ssid=${ssid} pass=${pass}`);
        json(res, { ok: true });
        setTimeout(() => broadcastWifi(WIFI.CONNECTING), 3000);
        if (pass === '123') setTimeout(() => broadcastWifi(WIFI.PROVISIONING, 1), 6000); /* 密码错误 */
        else setTimeout(() => broadcastWifi(WIFI.CONNECTED, 0), 6000);
      });
      return;
    }
    if (p === '/generate_204') { res.writeHead(204); res.end(); return; }
    if (p === '/hotspot-detect.html') { res.writeHead(200); res.end('<HTML><BODY>Success</BODY></HTML>'); return; }
    next();
  };
  const attachWs = (httpServer: any) => {
    // noServer mode: only handle /ws upgrade — vite's own HMR ws unaffected
    const wss = new WebSocketServer({ noServer: true });
    httpServer.on('upgrade', (req: any, socket: any, head: any) => {
      const url = new URL(req.url ?? '/', 'http://localhost');
      if (url.pathname !== '/ws') return; // not ours — pass through to vite (HMR etc.)
      wss.handleUpgrade(req, socket, head, ws => wss.emit('connection', ws, req));
    });
    wss.on('connection', (ws: WebSocket) => {
      console.log('[mock] ws connected');
      wsClients.add(ws);
      ws.send(Buffer.from([0x09, wifiState, failReason])); /* 推当前 wifi 状态 */
      ws.on('message', data => {
        const f = new Uint8Array(data as Buffer);
        switch (f[0]) {
          case 0x01: console.log(`[mock] kb mod=0x${f[1].toString(16)} keys=[${Array.from(f.slice(2, 8)).join(',')}]`); break;
          case 0x02: console.log(`[mock] mouse move dx=${(f[1] << 8 | f[2]) << 16 >> 16} dy=${(f[3] << 8 | f[4]) << 16 >> 16}`); break;
          case 0x03: console.log(`[mock] mouse btn=${f[1]} ${f[2] ? 'down' : 'up'}`); break;
          case 0x04: console.log(`[mock] sys key=0x${((f[1] << 8) | f[2]).toString(16)} ${f[3] ? 'down' : 'up'}`); break;
          case 0x06: {
            /* status query → reply 0x05 (floppy=0, mirrors /api/status) */
            console.log(`[mock] status query id=${f[1]}`);
            const r = new Uint8Array([0x05, 0x01, 0x00]);
            ws.send(r.buffer);
            break;
          }
          default: console.log(`[mock] frame 0x${f[0].toString(16)} len=${f.length}`);
        }
      });
      ws.on('close', () => { console.log('[mock] ws closed'); wsClients.delete(ws); });
    });
  };
  return {
    name: 'mock-api',
    configureServer(server) {
      server.middlewares.use(api);
      attachWs(server.httpServer);
    },
    configurePreviewServer(server) {
      server.middlewares.use(api);
      attachWs(server.httpServer);
    },
  };
}

export default defineConfig({
  root: __dirname,
  plugins: [preact(), mockApi(), viteSingleFile()],
  server: { port: 8899, strictPort: false }, // auto-pick next port if taken
  preview: { port: 8899, strictPort: false },
  build: {
    target: 'es2018',
    assetsInlineLimit: 100000000, // inline all assets (single-file fallback)
    cssCodeSplit: false,
  },
});
