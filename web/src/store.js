import { signal } from '@preact/signals';

/* ── 全局状态（preact/signals）── */
export const statusText = signal('连接中…');
export const dotColor = signal('#9a9386');
export const connected = signal(false);
export const capsLocked = signal(false);
export const floppyOn = signal(false);
export const floppyTitle = signal('软盘：未插入');
export const toastMsg = signal('');
export const toastErr = signal(false);
export const upBusy = signal(false);
export const upTxt = signal('');
export const shotBusy = signal(false);

/* ── WebSocket 协议层（原 main.js 原样搬运）── */
const MOD = { 0xe0: 0x01, 0xe1: 0x02, 0xe2: 0x04, 0xe3: 0x08, 0xe5: 0x20 }; /* ctrl shift alt gui rshift */

let ws = null;
let reconnectTimer = null;
let suppressReconnect = false;   /* 页面隐藏时主动关闭，禁止自动重连 */
let mod = 0;
const keys = new Set();

function setStatus(text, color) {
  statusText.value = text;
  dotColor.value = color;
}

export function isConnected() { return connected.value; }

export function connect() {
  /* guard: one socket */
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return;
  if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
  /* drop stale socket */
  if (ws) {
    ws.onopen = ws.onclose = ws.onerror = null;
    try { ws.close(); } catch (e) {}
    ws = null;
  }
  ws = new WebSocket('ws://' + location.host + '/ws');
  ws.binaryType = 'arraybuffer';
  ws.onopen = () => {
    /* clear stale keys */
    mod = 0; keys.clear();
    capsLocked.value = false;
    connected.value = true;
    sendKb();
    setStatus('已连接', '#4caf50');
    sendQuery(0x01);   /* 初始状态同步 */
  };
  ws.onmessage = e => {
    const f = new Uint8Array(e.data);
    if (f[0] === 0x05) handleStatusFrame(f);
  };
  ws.onclose = () => {
    connected.value = false;
    setStatus('已断开，重连中…', '#f44336');
    ws = null;
    if (!suppressReconnect)
      reconnectTimer = setTimeout(connect, 1000);
  };
}

/* 页面切走/熄屏 → 主动关 WS（释放连接槽，避免多开/僵尸占槽）；
 * 回到前台 → 立即重连（onopen 会重新同步状态）。
 * 多开时只有活动页面持有 WS 连接。 */
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'hidden') {
    suppressReconnect = true;
    if (ws) {
      ws.onopen = ws.onmessage = ws.onclose = ws.onerror = null;
      try { ws.close(); } catch (e) {}
      ws = null;
    }
    connected.value = false;
    setStatus('已断开', '#9a9386');   /* 页面切走：静默断开（不可见） */
  } else {
    suppressReconnect = false;
    connect();
  }
});

export function sendKb() {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(8);
  f[0] = 0x01; f[1] = mod;
  let i = 2; for (const k of keys) if (i < 8) f[i++] = k;
  ws.send(f.buffer);
}

export function keyEv(usage, down) {
  if (MOD[usage] !== undefined) {
    const b = MOD[usage];
    if (down) mod |= b; else mod &= ~b;
  } else {
    if (down) keys.add(usage); else keys.delete(usage);
  }
  sendKb();
}

export function sendMouseMove(dx, dy) {
  if ((dx === 0 && dy === 0) || !ws || ws.readyState !== 1) return;
  const f = new Uint8Array(5);
  f[0] = 0x02; f[1] = (dx >> 8) & 0xff; f[2] = dx & 0xff;
  f[3] = (dy >> 8) & 0xff; f[4] = dy & 0xff;
  ws.send(f.buffer);
}

export function sendMouseBtn(b, v) {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(3);
  f[0] = 0x03; f[1] = b; f[2] = v ? 1 : 0;
  ws.send(f.buffer);
}

export function sendSysKey(code, down) {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(4);
  f[0] = 0x04; f[1] = (code >> 8) & 0xff; f[2] = code & 0xff; f[3] = down ? 1 : 0;
  ws.send(f.buffer);
}

/* ── 状态查询（WS 代替 HTTP /api/status 轮询）──
 * 上行 0x06 [query_id]，0xFF=全部；下行 0x05 [query_id] [value...]
 * 0x01=floppy。可扩展：新状态 = 固件一个 case + 这里一个分支。 */
export function sendQuery(id) {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(2);
  f[0] = 0x06; f[1] = id;
  ws.send(f.buffer);
}

/* 进入 Recover/Update 模式（设备重启，跳过 USB Host，暴露 USJ） */
export function sendFlashMode() {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(1);
  f[0] = 0x07;
  ws.send(f.buffer);
}

/* 重启设备 */
export function sendReboot() {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(1);
  f[0] = 0x08;
  ws.send(f.buffer);
}

function handleStatusFrame(f) {
  if (f[1] === 0x01) {   /* floppy */
    const ins = f[2] === 1;
    floppyOn.value = ins;
    floppyTitle.value = ins ? '软盘：已插入' : '软盘：未插入';
  }
}

/* ── toast ── */
let toastTimer = null;
export function showToast(msg, isError) {
  toastMsg.value = msg;
  toastErr.value = !!isError;
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { toastMsg.value = ''; }, isError ? 6000 : 4000);
}

/* ── floppy 状态轮询：走 WS 查询帧（免 HTTP 建连开销，连接槽不常驻）── */
setInterval(() => {
  sendQuery(0x01);
}, 2000);

/* 启动连接 */
connect();
