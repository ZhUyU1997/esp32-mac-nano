import { signal } from '@preact/signals';

/* ── WiFi state machine (provisioning page) ──
 * Driven purely by HTTP polling of /api/status. No WebSocket here — the
 * provisioning page must not depend on WS (iOS CNA WebSheet has none, and
 * the WS drops during network switches). WebSocket lives in store.js and
 * is imported only by the remote page (index.html). */

export const wifiState = signal(null);

let wifiPollTimer = null;
function wifiPollStart() {
  if (wifiPollTimer) return;
  wifiPollTimer = setInterval(async () => {
    try {
      const r = await fetch('/api/status');
      const j = await r.json();
      if (j.state) {
        const next = { state: j.state, reason: j.reason || 0 };
        const cur = wifiState.value;
        /* publish only on change — re-rendering the form on every 1s tick
         * interrupts IME input on the password field (controlled input) */
        if (!cur || cur.state !== next.state || cur.reason !== next.reason)
          wifiState.value = next;
      }
    } catch {}
  }, 1000);
}
function wifiPollStop() {
  if (wifiPollTimer) {
    clearInterval(wifiPollTimer);
    wifiPollTimer = null;
  }
}

/* Provisioning-done flag: set when config is submitted; while CONNECTED the
 * success page shows. Reset on page reload (normal connect/refresh shows no hint). */
export const wifiProvisioned = signal(false);

/* Poll lifecycle self-contained (incl. page visibility): start on load,
 * hidden → stop, visible → resume. */
wifiPollStart();
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState === 'hidden') wifiPollStop();
  else wifiPollStart();
});
