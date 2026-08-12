import { useEffect, useRef, useState } from 'preact/hooks';
import { wifiState, wifiProvisioned } from './store.js';

/* Module-level timestamp + last SSID:
 * lastSsid is shown on the success page (valid until page reload). */
let lastSsid = '';
function WifiIcon({ level }) {
  /* Classic WiFi icon (dot + 3 arcs outward): 1 bar=dot, 2 bars=+inner arc, 3 bars=full.
   * Colors via CSS vars so they flip correctly in dark mode. */
  const on = Math.max(0, Math.min(3, level));
  const active = 'var(--icon-active)';
  const dim = 'var(--icon-dim)';
  return (
    <svg viewBox="0 0 1024 1024" width="19" height="19" aria-hidden>
      <path style={{ fill: on >= 1 ? active : dim }} d="M512 810.666667m-42.666667 0a42.666667 42.666667 0 1 0 85.333334 0 42.666667 42.666667 0 1 0-85.333334 0Z" />
      <path style={{ fill: on >= 2 ? active : dim }} d="M512 597.333333a213.333333 213.333333 0 0 0-148.053333 59.733334 42.666667 42.666667 0 1 0 59.306666 61.44 131.413333 131.413333 0 0 1 177.493334 0 42.666667 42.666667 0 1 0 59.306666-61.44A213.333333 213.333333 0 0 0 512 597.333333z" />
      <path style={{ fill: on >= 3 ? active : dim }} d="M512 384a384 384 0 0 0-276.053333 117.333333A42.666667 42.666667 0 0 0 298.666667 560.64a298.666667 298.666667 0 0 1 430.08 0 42.666667 42.666667 0 0 0 30.293333 12.8 42.666667 42.666667 0 0 0 30.72-72.106667A384 384 0 0 0 512 384z" />
      <path style={{ fill: on >= 3 ? active : dim }} d="M926.72 338.346667a597.333333 597.333333 0 0 0-829.44 0 42.666667 42.666667 0 0 0 58.88 61.44 512 512 0 0 1 711.68 0 42.666667 42.666667 0 0 0 29.44 11.946666 42.666667 42.666667 0 0 0 30.72-13.226666 42.666667 42.666667 0 0 0-1.28-60.16z" />
    </svg>
  );
}

function rssiLevel(rssi) {
  if (rssi >= -55) return 3;
  if (rssi >= -65) return 2;
  if (rssi >= -85) return 1; /* -77dBm maps to 1 bar (-75 was too strict) */
  return 0;
}

function Spinner() {
  return <div className="wifi-spinner" />;
}

/* Copy helper: the clipboard API only works in a secure context
 * (https/localhost); the real provisioning page is http://IP or
 * http://macnano.local (insecure) where navigator.clipboard is undefined,
 * so fall back to execCommand for copy + the checkmark feedback. */
async function copyText(text) {
  try {
    if (window.isSecureContext && navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
      return true;
    }
  } catch {}
  try {
    const ta = document.createElement('textarea');
    ta.value = text;
    ta.setAttribute('readonly', '');
    ta.style.position = 'fixed';
    ta.style.top = '-9999px';
    ta.style.opacity = '0';
    document.body.appendChild(ta);
    ta.focus(); /* iOS Safari needs focus for select/execCommand */
    ta.select();
    ta.setSelectionRange(0, ta.value.length);
    const ok = document.execCommand('copy');
    document.body.removeChild(ta);
    return ok;
  } catch {
    return false;
  }
}

/* Provisioning success page (rendered while CONNECTED + just provisioned): next steps. */
export function SuccessView() {
  const [ip, setIp] = useState('');
  const [copied, setCopied] = useState(false);
  const [copiedIp, setCopiedIp] = useState(false);
  const [countdown, setCountdown] = useState(20); /* matches the device 20s grace */
  const [closed, setClosed] = useState(false);
  /* The success page renders while the device STA is up: fetch /api/status
   * for the LAN IP (unicast works across bands/systems, no mDNS needed). */
  useEffect(() => {
    let alive = true;
    fetch('/api/status')
      .then(r => r.json())
      .then(j => { if (alive && j.sta) setIp(j.sta.ip || ''); })
      .catch(() => {});
    return () => { alive = false; };
  }, []);
  /* 20s countdown: the device closes the hotspot when grace expires (phone drops). */
  useEffect(() => {
    const t = setInterval(() => setCountdown(c => {
      if (c <= 1) { clearInterval(t); return 0; }
      return c - 1;
    }), 1000);
    return () => clearInterval(t);
  }, []);
  const copyIp = async () => {
    if (ip && (await copyText(ip))) {
      setCopiedIp(true);
      setTimeout(() => setCopiedIp(false), 2000);
    }
  };
  /* Copy IP + immediately close the hotspot (POST /api/wifi/done) */
  const copyAndClose = async () => {
    if (!ip) return;
    const ok = await copyText(ip);
    setCopied(ok);
    setTimeout(() => setCopied(false), 2000);
    try {
      const r = await fetch('/api/wifi/done', { method: 'POST' });
      if (r.ok) setClosed(true); /* only show "closed" after the device confirms */
      /* 409: device not in grace (e.g. mid-reconnect) — keep the button, don't mislead */
    } catch {
      /* network error: request may have arrived or link dropped — keep the button */
    }
    /* Real devices only (not localhost dev): close the page after copy+close so
     * the user can switch networks (window.close is browser-limited for
     * windows not opened by script — attempting is harmless) */
    const isDev = ['localhost', '127.0.0.1'].includes(location.hostname);
    if (!isDev) setTimeout(() => window.close(), 300);
  };
  return (
    <div className="wifi-center">
      <div className="wifi-success-ico" aria-hidden>
        <svg viewBox="0 0 24 24" width="34" height="34" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
          <circle cx="12" cy="12" r="10" />
          <path d="m8 12.5 2.7 2.7L16 9.5" />
        </svg>
      </div>
      <div className="wifi-center-title">已连接 Wi-Fi</div>
      <p className="wifi-hint">
        设备已连接<span className="wifi-ssid-name">{lastSsid}</span>
        <br />手机连接同一网络后，<br />用下面的 IP 地址访问设备
      </p>
      {closed ? (
        <div className="wifi-grace-msg">热点已关闭，请连接 {lastSsid} 后访问设备</div>
      ) : (
        ip && (
          <button type="button" className="wifi-done" onClick={copyAndClose}>
            <span>复制地址并关闭热点</span>
            {copied ? <span className="wifi-url-copied">已复制 ✓</span> : null}
          </button>
        )
      )}
      {ip && (
        <button type="button" className="wifi-ip" onClick={copyIp} aria-label="复制 IP 地址">
          <span className="wifi-ip-host">{ip}</span>
          {copiedIp ? (
            <span className="wifi-url-copied">已复制 ✓</span>
          ) : (
            <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden>
              <rect x="9" y="9" width="11" height="11" rx="2" />
              <path d="M5 15V5a2 2 0 0 1 2-2h10" />
            </svg>
          )}
        </button>
      )}
      {!closed && (
        <div className="wifi-grace-count">
          {countdown > 0
            ? `热点将在 ${countdown}s 后自动关闭`
            : `热点已自动关闭，请连接 ${lastSsid} 后访问设备`}
        </div>
      )}
    </div>
  );
}

/* Provisioning failure labels (device status reason: 1=bad password 2=no AP 3=other) */
const FAIL_TEXT = { 1: '密码错误', 2: '找不到网络', 3: '连接失败' };

/* Provisioning form: iOS Settings -> WiFi style (network list + password sheet) */
export function ProvisionView() {
  const [scan, setScan] = useState(null);       /* null=scanning, [] =empty */
  const [scanning, setScanning] = useState(false); /* refresh button state */
  const [sheet, setSheet] = useState(null);     /* AP currently in the sheet, null=closed */
  const [pass, setPass] = useState('');
  const passRef = useRef(null);
  /* focus the password input when the sheet opens (autoFocus attr only
   * works at page load for dynamically-created elements) */
  useEffect(() => {
    if (sheet) passRef.current?.focus();
  }, [sheet]);
  const [showPass, setShowPass] = useState(false);
  const [busy, setBusy] = useState(false);
  const [err, setErr] = useState('');
  /* Connecting/failed: shown inline in the wifi-cell (no page switch, no popup) */
  const [pending, setPending] = useState(null); /* { ssid, status: 'connecting' | 'failed' } */
  const pendingRef = useRef(null); /* the AP being submitted */
  const pendingTimerRef = useRef(null); /* 30s fallback */

  /* Enter connecting (confirmed or optimistic): close the sheet, status inline */
  function enterConnecting(ap, password) {
    wifiProvisioned.value = true;
    lastSsid = ap.ssid;
    pendingRef.current = { ap, pass: password };
    setPending({ ssid: ap.ssid, status: 'connecting' });
    setSheet(null);
    /* Fallback: device never reported (request lost) -> show failed inline after 30s */
    clearTimeout(pendingTimerRef.current);
    pendingTimerRef.current = setTimeout(() => {
      if (pendingRef.current) {
        pendingRef.current = null;
        setPending({ ssid: ap.ssid, status: 'failed' });
      }
    }, 30000);
  }

  /* Scan: poll until results arrive, then pause; refresh button re-scans */
  const timerRef = useRef(null);
  const seqRef = useRef(0);
  const failCountRef = useRef(0); /* consecutive network failures (reset by startScan) */
  const stopScan = () => { if (timerRef.current) { clearInterval(timerRef.current); timerRef.current = null; } };

  async function poll(seq) {
    try {
      const r = await fetch('/api/wifi/scan');
      const j = await r.json();
      if (seq !== seqRef.current) return; /* re-scanned or unmounted */
      if (j.scanning) {
        setScanning(true);
      } else {
        setScanning(false);
        setScan(Array.isArray(j) ? j : []);
        stopScan(); /* stop once we have data */
      }
    } catch {
      /* Network error (hotspot/httpd just started): retry a bounded number of
       * times. Do not treat it as an empty list (first load would show empty
       * until manual refresh), but do not poll forever either. */
      if (seq !== seqRef.current) return;
      if (++failCountRef.current >= 5) { /* 5 consecutive failures (~10s) -> stop, show retryable empty state */
        setScanning(false);
        setScan([]);
        stopScan();
        return;
      }
      setScanning(true); /* device not ready: keep retrying */
    }
  }

  function startScan() {
    stopScan();
    seqRef.current++;
    failCountRef.current = 0;
    const seq = seqRef.current;
    setScan(null);
    setScanning(true);
    poll(seq);
    timerRef.current = setInterval(() => poll(seq), 2000);
  }

  /* Auto-scan once (pause when data arrives); refresh button re-scans */
  useEffect(() => {
    startScan();
    return stopScan;
  }, []);

  /* State flow: connecting shown inline; wrong password re-opens the sheet;
   * other failures inline; success is routed by the app to the success page */
  useEffect(() => {
    const st = wifiState.value && wifiState.value.state;
    if (st === 'PROVISIONING' && pendingRef.current) {
      clearTimeout(pendingTimerRef.current);
      const p = pendingRef.current;
      pendingRef.current = null;
      const reason = (wifiState.value && wifiState.value.reason) || 3;
      setPending(null);
      if (reason === 1) {
        /* Wrong password: re-open the sheet for that AP, keep the typed password */
        setSheet(p.ap);
        setPass(p.pass);
        setErr('密码错误');
      } else {
        /* Other failures: stay on the page, show the specific error inline in the cell */
        setPending({ ssid: p.ap.ssid, status: 'failed', reason });
      }
    } else if (st === 'CONNECTED') {
      clearTimeout(pendingTimerRef.current);
      pendingRef.current = null;
      setPending(null);
    }
  }, [wifiState.value]);

  const busyRef = useRef(false); /* guard double-clicks: setState is async, busy can't stop back-to-back clicks */
  async function submit(ap, password) {
    if (busyRef.current) return;
    busyRef.current = true;
    setBusy(true);
    setErr('');
    const ctl = new AbortController();
    const t = setTimeout(() => ctl.abort(), 15000); /* httpd may be slow during network switch on device; avoid stuck "submitting" */
    try {
      const r = await fetch('/api/wifi/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid: ap.ssid, pass: password }),
        signal: ctl.signal,
      });
      const j = await r.json();
      if (!j.ok) {
        /* sheet errors show in the popup; direct-connect (no sheet) errors
         * surface inline via WS 0x09 fail_reason */
        if (sheet === ap) setErr(j.error || '提交失败');
      } else {
        enterConnecting(ap, password);
      }
    } catch {
      /* The device switches networks after receiving the request, so the
       * response may never arrive -> fetch fails. Enter connecting
       * optimistically: the actual result comes from the status poll. */
      enterConnecting(ap, password);
    } finally {
      clearTimeout(t);
      busyRef.current = false;
      setBusy(false); /* reset either way so the button never sticks */
    }
  }

  function openSheet(ap) {
    setSheet(ap);
    setPass('');
    setErr('');
    setShowPass(false);
  }

  return (
    <div className="wifi-page">
      <header className="wifi-nav"><h1>连接 Wi-Fi</h1></header>

      <div className="wifi-group">
        <div className="wifi-listhead">
          <span className="wifi-listtitle">可用网络</span>
          <button
            type="button"
            className={'wifi-refresh' + (scanning ? ' spin' : '')}
            onClick={startScan}
            aria-label="刷新网络列表">
            <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round" strokeLinejoin="round">
              <path d="M20 12a8 8 0 1 1-2.34-5.66" />
              <path d="M20 4v4h-4" />
            </svg>
          </button>
        </div>
        {scan === null ? (
          <div className="wifi-scanstate"><Spinner /></div>
        ) : scan.length === 0 ? (
          <div className="wifi-scanstate">
            <span>未发现 2.4GHz 网络<br />请确认路由器开启了 2.4GHz 频段</span>
          </div>
        ) : (
          scan.map(a => {
            const p = pending && pending.ssid === a.ssid ? pending : null;
            const isConnecting = p && p.status === 'connecting';
            const isFailed = p && p.status === 'failed';
            return (
              <button
                key={a.ssid}
                type="button"
                className={'wifi-cell' + (isConnecting ? ' connecting' : '') + (isFailed ? ' failed' : '')}
                onClick={() => {
                  if (p) setPending(null);
                  if (a.auth === 0) {
                    /* open network: no password, connect directly */
                    submit(a, '');
                  } else {
                    openSheet(a);
                  }
                }}
                disabled={pending !== null && !isConnecting && !isFailed}>
                <span className="wifi-signal"><WifiIcon level={rssiLevel(a.rssi)} /></span>
                <span className="wifi-cell-body">
                  <span className="wifi-cell-main">
                    <span className="wifi-ssid">{a.ssid}</span>
                  </span>
                  {isConnecting && (
                    <span className="wifi-cell-status">正在连接…</span>
                  )}
                  {isFailed && (
                    <span className="wifi-cell-status failed">{FAIL_TEXT[p.reason] || '连接失败'}</span>
                  )}
                </span>
                {a.auth > 1 && (
                  <span className="wifi-lock" aria-label="加密网络">
                    <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                      <rect x="5" y="11" width="14" height="9" rx="2" />
                      <path d="M8 11V7a4 4 0 0 1 8 0v4" />
                    </svg>
                  </span>
                )}
              </button>
            );
          })
        )}
      </div>

      {sheet && (
        <div className="wifi-mask" onClick={() => setSheet(null)}>
          <div className="wifi-sheet" onClick={e => e.stopPropagation()}>
            <div className="wifi-sheet-title">{sheet.ssid}</div>
            <div className="wifi-sheet-row">
              <label>密码</label>
              <input
                ref={passRef}
                type={showPass ? 'text' : 'password'}
                value={pass}
                placeholder="请输入密码" /* sheet only opens for secured nets (auth=0 connects directly) */
                onChange={e => setPass(e.target.value)}
              />
              <button type="button" className="wifi-eye" onClick={() => setShowPass(!showPass)} aria-label="显示/隐藏密码">
                {showPass ? (
                  /* open eye: currently visible, click to hide */
                  <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                    <path d="M2.5 12S6 5.5 12 5.5 21.5 12 21.5 12 18 18.5 12 18.5 2.5 12 2.5 12Z" />
                    <circle cx="12" cy="12" r="3" />
                  </svg>
                ) : (
                  /* closed eye (slash): currently hidden, click to show */
                  <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                    <path d="M2.5 12S6 5.5 12 5.5 21.5 12 21.5 12 18 18.5 12 18.5 2.5 12 2.5 12Z" />
                    <circle cx="12" cy="12" r="3" />
                    <path d="M4 20 20 4" />
                  </svg>
                )}
              </button>
            </div>
            {err && <div className="wifi-sheet-err">{err}</div>}
            <div className="wifi-sheet-actions">
              <button type="button" className="wifi-sheet-btn cancel" onClick={() => setSheet(null)}>取消</button>
              <button type="button" className="wifi-sheet-btn conn" disabled={busy} onClick={() => submit(sheet, pass)}>
                {busy ? '提交中…' : '连接'}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
