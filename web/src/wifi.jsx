import { useEffect, useRef, useState } from 'preact/hooks';
import { wifiState, wifiProvisioned } from './store.js';

/* 配网提交时间戳与最近 SSID（模块级）——
 * lastSsid：配网完成页显示设备连接的网络（页面重载前有效） */
let lastSsid = '';
function WifiIcon({ level }) {
  /* 经典 WiFi 图标（点 + 3 弧从内到外）：1 格=点，2 格=点+内弧，3 格=全亮 */
  const on = Math.max(0, Math.min(3, level));
  const lit = '#1a1a1a';
  const dim = '#d8d8d8';
  return (
    <svg viewBox="0 0 1024 1024" width="19" height="19" aria-hidden>
      <path fill={on >= 1 ? lit : dim} d="M512 810.666667m-42.666667 0a42.666667 42.666667 0 1 0 85.333334 0 42.666667 42.666667 0 1 0-85.333334 0Z" />
      <path fill={on >= 2 ? lit : dim} d="M512 597.333333a213.333333 213.333333 0 0 0-148.053333 59.733334 42.666667 42.666667 0 1 0 59.306666 61.44 131.413333 131.413333 0 0 1 177.493334 0 42.666667 42.666667 0 1 0 59.306666-61.44A213.333333 213.333333 0 0 0 512 597.333333z" />
      <path fill={on >= 3 ? lit : dim} d="M512 384a384 384 0 0 0-276.053333 117.333333A42.666667 42.666667 0 0 0 298.666667 560.64a298.666667 298.666667 0 0 1 430.08 0 42.666667 42.666667 0 0 0 30.293333 12.8 42.666667 42.666667 0 0 0 30.72-72.106667A384 384 0 0 0 512 384z" />
      <path fill={on >= 3 ? lit : dim} d="M926.72 338.346667a597.333333 597.333333 0 0 0-829.44 0 42.666667 42.666667 0 0 0 58.88 61.44 512 512 0 0 1 711.68 0 42.666667 42.666667 0 0 0 29.44 11.946666 42.666667 42.666667 0 0 0 30.72-13.226666 42.666667 42.666667 0 0 0-1.28-60.16z" />
    </svg>
  );
}

function rssiLevel(rssi) {
  if (rssi >= -55) return 3;
  if (rssi >= -65) return 2;
  if (rssi >= -85) return 1; /* -77dBm 显示 1 格（原来 -75 太苛刻） */
  return 0;
}

function Spinner() {
  return <div className="wifi-spinner" />;
}

/* 复制工具：clipboard API 仅在 secure context（https/localhost）可用；
 * 真机配网页是 http://IP 或 http://macnano.local（insecure context），
 * navigator.clipboard 为 undefined → 回退 execCommand 才能复制并显示 ✓。 */
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

/* 配网完成提示页（App 在 CONNECTED + 刚配网成功时渲染）：提示下一步 */
export function SuccessView() {
  const [ip, setIp] = useState('');
  const [copied, setCopied] = useState(false);
  const [copiedIp, setCopiedIp] = useState(false);
  const [countdown, setCountdown] = useState(20); /* 与设备 grace 20s 同步 */
  const [closed, setClosed] = useState(false);
  /* 成功页在设备 STA 已连时渲染：拉 /api/status 拿局域网 IP
   * （IP 单播跨频段/跨系统都可用，不依赖 mDNS 解析） */
  useEffect(() => {
    let alive = true;
    fetch('/api/status')
      .then(r => r.json())
      .then(j => { if (alive && j.sta) setIp(j.sta.ip || ''); })
      .catch(() => {});
    return () => { alive = false; };
  }, []);
  /* 20s 倒计时：设备 grace 到点自动关热点（手机断连、WS 断） */
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
  /* 复制 IP + 立即关闭配网热点（设备端 POST /api/wifi/done） */
  const copyAndClose = async () => {
    if (!ip) return;
    const ok = await copyText(ip);
    setCopied(ok);
    setTimeout(() => setCopied(false), 2000);
    try {
      const r = await fetch('/api/wifi/done', { method: 'POST' });
      if (r.ok) setClosed(true); /* 设备确认已关热点才显示"已关闭" */
      /* 409：设备不在宽限期（如恰逢断线）——保持按钮，不误导 */
    } catch {
      /* 网络错误：请求可能已到达或连接已断——保守保持按钮 */
    }
    /* 仅真实环境（非 localhost dev）：复制+关热点后主动关闭页面，用户直接切网
     * （window.close 受浏览器限制：脚本未打开的窗口可能被拒——尝试无害） */
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
      <div className="wifi-center-title">配网完成</div>
      <p className="wifi-hint">
        设备已连接<span className="wifi-ssid-name">{lastSsid}</span>
        <br />手机连接同一网络后，<br />用下面的 IP 地址访问设备
      </p>
      {closed ? (
        <div className="wifi-grace-msg">热点已关闭，请连接 {lastSsid} 后访问设备</div>
      ) : (
        ip && (
          <button type="button" className="wifi-done" onClick={copyAndClose}>
            <span>复制并关闭配网热点</span>
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
            ? `配网热点将在 ${countdown}s 后自动关闭`
            : `热点已自动关闭，请连接 ${lastSsid} 后访问设备`}
        </div>
      )}
    </div>
  );
}

/* 配网失败细分文案（设备 0x09 帧第 3 字节：1=密码错误 2=找不到网络 3=其他） */
const FAIL_TEXT = { 1: '密码错误', 2: '找不到网络', 3: '连接失败' };

/* 配网表单：iOS 设置 → WiFi 风格（网络列表 + 底部弹窗输密码） */
export function ProvisionView() {
  const [scan, setScan] = useState(null);       /* null=扫描中 [] =空 */
  const [scanning, setScanning] = useState(false); /* 刷新按钮状态 */
  const [sheet, setSheet] = useState(null);     /* 当前弹窗的 AP，null=收起 */
  const [pass, setPass] = useState('');
  const [showPass, setShowPass] = useState(false);
  const [busy, setBusy] = useState(false);
  const [err, setErr] = useState('');
  /* 连接中/失败：状态显示在 wifi-cell 列表项内（不切页面、不弹窗） */
  const [pending, setPending] = useState(null); /* { ssid, status: 'connecting' | 'failed' } */
  const pendingRef = useRef(null); /* 提交中的 ap */
  const pendingTimerRef = useRef(null); /* 30s 兜底 */

  /* 进入连接中（提交成功或乐观路径）：收起 sheet，状态显示在列表项内 */
  function enterConnecting(ap, password) {
    wifiProvisioned.value = true;
    lastSsid = ap.ssid;
    pendingRef.current = { ap, pass: password };
    setPending({ ssid: ap.ssid, status: 'connecting' });
    setSheet(null);
    /* 兜底：设备一直没推状态（请求真丢了）→ 30s 后行内显示失败 */
    clearTimeout(pendingTimerRef.current);
    pendingTimerRef.current = setTimeout(() => {
      if (pendingRef.current) {
        pendingRef.current = null;
        setPending({ ssid: ap.ssid, status: 'failed' });
      }
    }, 30000);
  }

  /* 扫描：轮询直到拿到结果就暂停；点击刷新按钮可重新扫描 */
  const timerRef = useRef(null);
  const seqRef = useRef(0);
  const failCountRef = useRef(0); /* 连续网络失败计数（startScan 清零） */
  const stopScan = () => { if (timerRef.current) { clearInterval(timerRef.current); timerRef.current = null; } };

  async function poll(seq) {
    try {
      const r = await fetch('/api/wifi/scan');
      const j = await r.json();
      if (seq !== seqRef.current) return; /* 已重新扫描或已卸载 */
      if (j.scanning) {
        setScanning(true);
      } else {
        setScanning(false);
        setScan(Array.isArray(j) ? j : []);
        stopScan(); /* 有数据就暂停 */
      }
    } catch {
      /* 网络错误（设备热点/httpd 刚启动）：限次重试。不能当成空列表停止
       * （否则首次进入显示空，要点刷新才出来）；但也不能无限轮询。 */
      if (seq !== seqRef.current) return;
      if (++failCountRef.current >= 5) { /* 连续 5 次失败（~10s）→ 停止，显示可重试空态 */
        setScanning(false);
        setScan([]);
        stopScan();
        return;
      }
      setScanning(true); /* 设备未就绪：继续重试 */
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

  /* 自动扫描一次（有数据暂停），用户点刷新按钮可重新扫描 */
  useEffect(() => {
    startScan();
    return stopScan;
  }, []);

  /* 状态流转：连接中在行内显示；密码错误重弹输入框；其他失败行内显示；成功由 App 跳提示页 */
  useEffect(() => {
    const st = wifiState.value && wifiState.value.state;
    if (st === 'PROVISIONING' && pendingRef.current) {
      clearTimeout(pendingTimerRef.current);
      const p = pendingRef.current;
      pendingRef.current = null;
      const reason = (wifiState.value && wifiState.value.reason) || 3;
      setPending(null);
      if (reason === 1) {
        /* 密码错误：重新弹出该网络密码输入框，保留已输密码直接改 */
        setSheet(p.ap);
        setPass(p.pass);
        setErr('密码错误');
      } else {
        /* 其他失败：留在配网页，该 wifi-cell 行内显示细分错误 */
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
    const t = setTimeout(() => ctl.abort(), 15000); /* 真机切网时 httpd 可能慢，防永久"提交中…" */
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
      /* 设备收到请求后切换网络，响应可能发不出来 → fetch 失败。
       * 乐观进入连接中：实际结果由 WS 状态推送决定。 */
      enterConnecting(ap, password);
    } finally {
      clearTimeout(t);
      busyRef.current = false;
      setBusy(false); /* 无论成败都复位，按钮不会卡住 */
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
      <header className="wifi-nav"><h1>WiFi 配网</h1></header>

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
                type={showPass ? 'text' : 'password'}
                value={pass}
                placeholder="请输入密码" /* sheet only opens for secured nets (auth=0 open goes direct) */
                onChange={e => setPass(e.target.value)}
                autoFocus
              />
              <button type="button" className="wifi-eye" onClick={() => setShowPass(!showPass)} aria-label="显示/隐藏密码">
                {showPass ? (
                  /* 睁眼：当前可见，点击隐藏 */
                  <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                    <path d="M2.5 12S6 5.5 12 5.5 21.5 12 21.5 12 18 18.5 12 18.5 2.5 12 2.5 12Z" />
                    <circle cx="12" cy="12" r="3" />
                  </svg>
                ) : (
                  /* 闭眼（斜杠）：当前隐藏，点击显示 */
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
