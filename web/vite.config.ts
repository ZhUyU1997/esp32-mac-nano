// Vite 配置：dev = mock serve（API/WS 中间件），build = 单文件 index.html（vite-plugin-singlefile）
import { defineConfig, type Plugin } from 'vite';
import preact from '@preact/preset-vite';
import { viteSingleFile } from 'vite-plugin-singlefile';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { PNG } from 'pngjs';
import { WebSocketServer, type WebSocket } from 'ws';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// 截屏帧：真实图片（640x480 RGBA PNG）→ 灰度阈值 → 1-bit（1=黑 0=白）——同原 server.ts
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

// mock API 中间件 + WebSocket（原 server.ts 逻辑）
function mockApi(): Plugin {
  const api = (req: any, res: any, next: () => void) => {
    const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
    const p = url.pathname;
    if (p === '/api/status') return json(res, { floppy: false });
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
    if (p === '/generate_204') { res.writeHead(204); res.end(); return; }
    if (p === '/hotspot-detect.html') { res.writeHead(200); res.end('<HTML><BODY>Success</BODY></HTML>'); return; }
    next();
  };
  const attachWs = (httpServer: any) => {
    // noServer 模式：只接管 /ws 的 upgrade——vite 自己的 HMR ws 不受影响
    const wss = new WebSocketServer({ noServer: true });
    httpServer.on('upgrade', (req: any, socket: any, head: any) => {
      const url = new URL(req.url ?? '/', 'http://localhost');
      if (url.pathname !== '/ws') return; // 非我们的——交给 vite（HMR 等）
      wss.handleUpgrade(req, socket, head, ws => wss.emit('connection', ws, req));
    });
    wss.on('connection', (ws: WebSocket) => {
      console.log('[mock] ws connected');
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
      ws.on('close', () => console.log('[mock] ws closed'));
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
  server: { port: 8899, strictPort: false }, // 被占时自动换下一个端口
  preview: { port: 8899, strictPort: false },
  build: {
    target: 'es2018',
    assetsInlineLimit: 100000000, // 内联所有资源（单文件兜底）
    cssCodeSplit: false,
  },
});
