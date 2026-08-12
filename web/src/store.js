import { signal } from '@preact/signals';

/* ── Global state (preact/signals) ── */
export const statusText = signal('连接中…');
export const dotColor = signal('#9a9386');
export const connected = signal(false);
export const capsLocked = signal(false);
export const toastMsg = signal('');
export const toastErr = signal(false);
export const upBusy = signal(false);
export const upTxt = signal('');
export const shotBusy = signal(false);

/* 配网完成标记：提交 config 成功置位；CONNECTED 时显示"配网完成"提示页
 * Reset on page reload (normal connect/refresh shows no hint). */
export const wifiProvisioned = signal(false);

/* ── WebSocket protocol layer (moved verbatim from original main.js) ── */
const MOD = { 0xe0: 0x01, 0xe1: 0x02, 0xe2: 0x04, 0xe3: 0x08, 0xe5: 0x20 }; /* ctrl shift alt gui rshift */

let ws = null;
let reconnectTimer = null;
let suppressReconnect = false;   /* closed on page hide, no auto-reconnect */
let mod = 0;
const keys = new Set();

function setStatus(text, color) {
  statusText.value = text;
  dotColor.value = color;
}

export function isConnected() { return connected.value; }

/* WS lifecycle: the remote page starts it on load (main.jsx ensureWs);
 * the provisioning page (provision.html) never imports this module.
 * Reconnect after background restore is handled in the visibilitychange
 * handler below (hidden closes the socket, visible reconnects) — the
 * remote page is the only importer of this module. */
export function ensureWs() { connect(); }  /* connect() is idempotent */

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
    lastStatusReply = Date.now();   /* start a fresh heartbeat window — the old
                                     * value would kill a just-reconnected socket */
    sendKb();
    setStatus('已连接', '#4caf50');
    sendQuery(0x01);   /* initial state sync */
  };
  ws.onmessage = e => {
    const f = new Uint8Array(e.data);
    if (f[0] === 0x05) { lastStatusReply = Date.now(); handleStatusFrame(f); }
    /* wifi state (0x09) is not handled here — the provisioning page polls
     * /api/status; this module is remote-control only */
  };
  ws.onclose = () => {
    connected.value = false;
    setStatus('已断开，重连中…', '#f44336');
    ws = null;
    if (!suppressReconnect)
      reconnectTimer = setTimeout(connect, 1000);
  };
}

/* Page hidden/screen off → close WS proactively (free connection slot, avoid multiple
 * zombie tabs); back to foreground → reconnect immediately (onopen re-syncs state).
 * Only the active page holds the WS connection. */
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'hidden') {
    suppressReconnect = true;
    if (ws) {
      ws.onopen = ws.onmessage = ws.onclose = ws.onerror = null;
      try { ws.close(); } catch (e) {}
      ws = null;
    }
    connected.value = false;
    setStatus('已断开', '#9a9386');   /* page hidden: silent close (invisible) */
  } else {
    suppressReconnect = false;
    connect(); /* reconnect after returning from background (remote page only) */
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

/* Click semantics: the C side guarantees the press duration (down +
 * delayed release) so taps register even when WS delivery jitter
 * compresses the frames. Drag hold/release still uses sendMouseBtn. */
export function sendClick() {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(1);
  f[0] = 0x0A;
  ws.send(f.buffer);
}

export function sendSysKey(code, down) {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(4);
  f[0] = 0x04; f[1] = (code >> 8) & 0xff; f[2] = code & 0xff; f[3] = down ? 1 : 0;
  ws.send(f.buffer);
}

/* ── Status query (WS replaces HTTP /api/status polling) ──
 * Up 0x06 [query_id], 0xFF=all; down 0x05 [query_id] [value...]
 * 0x01=floppy. Extensible: new status = one firmware case + one branch here. */
export function sendQuery(id) {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(2);
  f[0] = 0x06; f[1] = id;
  ws.send(f.buffer);
}

/* Enter Recover/Update mode (device restarts, skips USB Host, exposes USJ) */
export function sendFlashMode() {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(1);
  f[0] = 0x07;
  ws.send(f.buffer);
}

/* Reboot device */
export function sendReboot() {
  if (!ws || ws.readyState !== 1) return;
  const f = new Uint8Array(1);
  f[0] = 0x08;
  ws.send(f.buffer);
}

function handleStatusFrame(f) {
  if (f[1] === 0x01) {   /* floppy — no longer surfaced in the UI */
  }
}

/* App-level heartbeat: the device answers status queries (0x06 -> 0x05)
 * every 2s. If no reply for HEARTBEAT_TIMEOUT_MS the link is dead (device
 * rebooting / flashing / crashed) even though TCP hasn't noticed —
 * force-close the socket so the status bar flips to "disconnected" and
 * reconnects promptly instead of staying "connected" on a half-open link. */
const HEARTBEAT_TIMEOUT_MS = 2000;
let lastStatusReply = Date.now();
setInterval(() => {
  /* Only when the socket is actually open: during reconnect (CONNECTING)
   * there is no 0x05 reply by definition — the heartbeat must not keep
   * firing on the stale lastStatusReply or it kills every reconnect. */
  if (!ws || ws.readyState !== WebSocket.OPEN) return;
  if (Date.now() - lastStatusReply > HEARTBEAT_TIMEOUT_MS) {
    /* Update the status bar synchronously: ws.close() may not reliably
     * fire onclose (e.g. a dead device leaves a half-open socket), so
     * relying on onclose left the bar stuck on "已连接". */
    connected.value = false;
    setStatus('已断开，重连中…', '#f44336');
    if (ws) {
      ws.onopen = ws.onmessage = ws.onclose = ws.onerror = null;
      try { ws.close(); } catch (e) {}
      ws = null;
    }
    if (!suppressReconnect) {
      reconnectTimer = setTimeout(connect, 1000);
    }
  }
}, 2000);

/* ── toast ── */
let toastTimer = null;
export function showToast(msg, isError) {
  toastMsg.value = msg;
  toastErr.value = !!isError;
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { toastMsg.value = ''; }, isError ? 6000 : 4000);
}

/* ── Floppy status polling: via WS query frames (no HTTP connection overhead, no persistent connection slot) ── */
setInterval(() => {
  sendQuery(0x01);
}, 2000);

/* Start connection */
connect();
