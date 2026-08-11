/* ESP32-Mac web flasher — Preact component, esptool-js over WebSerial.
 * Target: ESP32-S3 in "Recover / Update" mode (USB Host skipped,
 * USB-Serial-JTAG exposed). esptool-js detects the USB-JTAG-Serial PID
 * (303a:1001) and auto-uses the USJ DTR/RTS reset sequence. */
import { render } from 'preact';
import { useState, useRef, useCallback, useMemo, useEffect } from 'preact/hooks';
import { ESPLoader, Transport } from 'esptool-js';
import JSZip from 'jszip';

/* ── Helpers ────────────────────────────────────────────────────── */

async function sha256Hex(data) {
  const digest = await crypto.subtle.digest('SHA-256', data);
  return Array.from(new Uint8Array(digest), (b) => b.toString(16).padStart(2, '0')).join('');
}

async function loadFirmwarePackage(file, log) {
  const zip = await JSZip.loadAsync(file);
  const mf = zip.file('manifest.json');
  if (!mf) throw new Error('zip 中没有 manifest.json');
  const manifest = JSON.parse(await mf.async('string'));
  if (!Array.isArray(manifest.files) || !manifest.files.length) {
    throw new Error('manifest.json 格式错误（无 files）');
  }
  const entries = [];
  for (const f of manifest.files) {
    const zf = zip.file(f.name);
    if (!zf) throw new Error(`zip 中缺少 ${f.name}`);
    const data = await zf.async('uint8array');
    if (f.size !== undefined && data.length !== f.size) {
      throw new Error(`${f.name} 大小不符 (${data.length} != ${f.size})`);
    }
    if (f.sha256) {
      const hash = await sha256Hex(data);
      if (hash !== f.sha256) throw new Error(`${f.name} SHA256 校验失败`);
    }
    entries.push({ address: parseInt(f.address, 16), name: f.name, data });
    log(`  固件包: ${f.address} ${f.name} (${data.length}B)`);
  }
  return { entries, manifest };
}

/* ── Inline icons (stroke, currentColor) ───────────────────────── */

const Svg = ({ size = 16, children }) => (
  <svg width={size} height={size} viewBox="0 0 24 24" fill="none"
    stroke="currentColor" strokeWidth={2} strokeLinecap="round" strokeLinejoin="round"
    aria-hidden="true">{children}</svg>
);
const UsbIcon = (p) => (<Svg {...p}><path d="M12 3v7" /><rect x="8" y="10" width="8" height="11" rx="2" /><path d="M12 15v2" /></Svg>);
const FileIcon = (p) => (<Svg {...p}><path d="M6 2h8l4 4v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2Z" /><path d="M14 2v4h4" /></Svg>);
const ZapIcon = (p) => (<Svg {...p}><path d="M13 2 4 14h6l-1 8 9-12h-6l1-8Z" /></Svg>);
const ChipIcon = (p) => (<Svg {...p}><rect x="6" y="6" width="12" height="12" rx="2" /><path d="M9 2v4M15 2v4M9 18v4M15 18v4M2 9h4M2 15h4M18 9h4M18 15h4" /></Svg>);
const AlertIcon = (p) => (<Svg {...p}><circle cx="12" cy="12" r="10" /><path d="M12 8v5" /><path d="M12 16.5v.01" /></Svg>);

/* theme-toggle icons (inline strings — button lives in static titlebar HTML) */
const ICON_SUN = '<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>';
const ICON_MOON = '<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8Z"/></svg>';

/* ── Main component ─────────────────────────────────────────────── */

function FlashTool() {
  /* WebSerial support gate: only desktop Chrome/Edge expose navigator.serial,
   * and only in a secure context (HTTPS or localhost). Detect upfront so
   * unsupported browsers get a clear message instead of a dead button. */
  /* Order matters: Chrome hides navigator.serial in non-secure contexts
   * (file://, http://) — check secure context FIRST, otherwise an HTTPS
   * requirement would be misreported as "browser unsupported". */
  const secureOk = typeof window === 'undefined' || window.isSecureContext !== false;
  const serialOk = typeof navigator !== 'undefined' && 'serial' in navigator;

  const [conn, setConn] = useState('idle');       /* idle | connecting | connected */
  const [dev, setDev] = useState(null);           /* {chip, flash} | null */
  const [banner, setBanner] = useState(null);
  const [pkg, setPkg] = useState(null);           /* {version, entries, settings} | null */
  const [pkgError, setPkgError] = useState('');
  const [logOpen, setLogOpen] = useState(false);
  const [busy, setBusy] = useState(false);
  const [flashed, setFlashed] = useState(false);   /* whether flashing completed this session */
  const [progress, setProgress] = useState(null); /* {pct, text} | null */
  const [dragOver, setDragOver] = useState(false);
  const [copied, setCopied] = useState(false);
  /* theme: head script already set html[data-theme] before paint */
  const [theme, setTheme] = useState(() =>
    document.documentElement.getAttribute('data-theme') === 'dark' ? 'dark' : 'light');

  const logRef = useRef(null);
  const loaderRef = useRef(null);
  const portRef = useRef(null);

  const log = useCallback((text, isErr = false) => {
    const el = logRef.current;
    if (!el) return;
    const line = document.createElement('div');
    if (isErr) line.className = 'err';
    line.textContent = text;
    el.appendChild(line);
    el.scrollTop = el.scrollHeight;
  }, []);

  /* theme toggle button (static titlebar HTML, wired here) */
  useEffect(() => {
    const btn = document.getElementById('theme-toggle');
    if (!btn) return;
    btn.innerHTML = theme === 'dark' ? ICON_SUN : ICON_MOON;
    const toggle = () => {
      const next = theme === 'dark' ? 'light' : 'dark';
      setTheme(next);
      document.documentElement.setAttribute('data-theme', next);
      try { localStorage.setItem('flash-theme', next); } catch (e) { /* ignore */ }
    };
    btn.addEventListener('click', toggle);
    return () => btn.removeEventListener('click', toggle);
  }, [theme]);

  /* title-bar status dot + label follow connection state:
   * idle = grey / connecting = amber blink / connected = steady green */
  useEffect(() => {
    const dot = document.querySelector('.titlebar .dot');
    if (dot) dot.className = 'dot' + (conn === 'connecting' ? ' busy' : conn === 'connected' ? ' ok' : '');
    const label = document.getElementById('conn-state');
    if (label) {
      label.textContent = conn === 'connecting' ? '连接中…' : conn === 'connected' ? '已连接' : '未连接';
      label.className = 'conn' + (conn === 'connecting' ? ' busy' : conn === 'connected' ? ' ok' : '');
    }
  }, [conn]);

  const terminal = useMemo(() => ({
    clean: () => { if (logRef.current) logRef.current.innerHTML = ''; },
    writeLine: (d) => log(d),
    write: (d) => log(d),
  }), [log]);

  /* firmware source: firmware package zip (manual partition feature removed for now, may return later) */
  const hasFw = !!pkg;

  const fmtSize = (n) => (n >= 1024 ? `${(n / 1024).toFixed(0)} KB` : `${n} B`);

  /* one caption of what will be written — replaces scattered hints */
  const fwSummary = () => {
    if (!pkg) return '';
    const total = pkg.entries.reduce((s, f) => s + f.data.length, 0);
    return `将写入 ${pkg.entries.length} 个分区，共 ${fmtSize(total)}`;
  };

  async function applyFirmwareFile(file) {
    if (!file) return;
    setPkgError('');
    setBanner(null);
    setFlashed(false);     /* new firmware → flash flow restarts */
    try {
      const { entries, manifest } = await loadFirmwarePackage(file, log);
      setPkg({ version: manifest.version, entries, settings: manifest.flash_settings ?? null });
      log(`固件包 v${manifest.version} 加载完成（${entries.length} 个分区，SHA256 ✓）`);
    } catch (err) {
      setPkg(null);
      setPkgError(`加载失败: ${err.message}`);
      setBanner(`固件包加载失败: ${err.message}`);
      log(`固件包加载失败: ${err.message}`, true);
    }
  }

  function onPkgChange(e) {
    const file = e.target.files?.[0];
    e.target.value = '';   /* allow re-picking the same file */
    applyFirmwareFile(file);
  }

  /* ── Log actions ─────────────────────────────────────────────── */

  function onCopyLog(e) {
    e.stopPropagation();   /* don't toggle the <details> via summary click */
    const text = logRef.current?.innerText ?? '';
    if (!text) return;
    navigator.clipboard?.writeText(text)
      .then(() => { setCopied(true); setTimeout(() => setCopied(false), 1500); })
      .catch(() => {});
  }

  function onClearLog(e) {
    e.stopPropagation();
    if (logRef.current) logRef.current.innerHTML = '';
  }

  /* latest-state mirror for event listeners (closures see stale values) */
  const stateRef = useRef({ conn: 'idle', busy: false, flashed: false, pct: 0 });
  useEffect(() => {
    stateRef.current = { conn, busy, flashed, pct: progress?.pct ?? 0 };
  }, [conn, busy, flashed, progress]);

  /* USB pulled / power cut mid-flash: Web Serial fires disconnect immediately.
   * Detect it right away instead of waiting for esptool-js to hang or time out.
   * Ignore when flashed (device hard_reset after write is an expected drop). */
  const onSerialDisconnect = useCallback(() => {
    const { conn, busy, flashed, pct } = stateRef.current;
    if (conn === 'idle' || flashed) return;
    log('USB 连接已断开', true);
    setLogOpen(true);
    setConn('idle');
    setDev(null);
    portRef.current = null;
    loaderRef.current = null;
    if (busy && pct < 100) {
      setBusy(false);
      setProgress((prev) => ({ pct: prev?.pct ?? 0, error: true, text: 'USB 已断开，烧写中断' }));
      setBanner('烧写中断：USB 连接已断开，请重新连接设备后重试');
    } else {
      setBanner('USB 连接已断开');
    }
  }, [log]);

  useEffect(() => {
    navigator.serial?.addEventListener('disconnect', onSerialDisconnect);
    return () => navigator.serial?.removeEventListener('disconnect', onSerialDisconnect);
  }, [onSerialDisconnect]);

  /* ── Connect ─────────────────────────────────────────────────── */

  async function connect() {
    setBanner(null);
    setConn('connecting');
    try {
      const port = await navigator.serial.requestPort({ filters: [{ usbVendorId: 0x303a }] });
      portRef.current = port;
      /* no manual port.open() — esptool-js Transport.connect() opens it itself */
      const loader = new ESPLoader({
        transport: new Transport(port),
        baudrate: 460800,
        terminal,
        chipType: 'esp32s3',
      });
      loaderRef.current = loader;
      log('正在连接设备…');
      await loader.main();   /* detect USJ PID → DTR/RTS handshake reset → connect */
      setConn('connected');
      setFlashed(false);
      setLogOpen(true);
      log('连接成功 — 可加载固件包');
      /* device info */
      const chip = loader.chip?.CHIP_NAME ?? 'ESP32-S3';
      let flashSize = '?';
      try { flashSize = await loader.detectFlashSize(); } catch { /* ignore */ }
      setDev({ chip, flash: flashSize });
    } catch (err) {
      setConn('idle');
      setBanner(`连接失败: ${err?.message ?? err}`);
      log(`连接失败: ${err?.message ?? err}`, true);
      if (portRef.current) {
        try { portRef.current.close(); } catch { /* ignore */ }
        portRef.current = null;
      }
      loaderRef.current = null;
    }
  }

  async function disconnect() {
    try { await loaderRef.current?.disconnect(); } catch { /* ignore */ }
    if (portRef.current) {
      try { portRef.current.close(); } catch { /* ignore */ }
      portRef.current = null;
    }
    loaderRef.current = null;
    setConn('idle');
    setDev(null);
    setFlashed(false);
    setLogOpen(false);
    log('已断开连接');
  }

  /* flashing done: device rebooted via hard_reset, USB dropped — proactively end the connection.
   * keep flashed / progress (“flashing complete” feedback), only reset connection state. */
  async function finishAfterFlash() {
    try { await loaderRef.current?.disconnect(); } catch { /* ignore */ }
    if (portRef.current) {
      try { portRef.current.close(); } catch { /* ignore */ }
      portRef.current = null;
    }
    loaderRef.current = null;
    setConn('idle');
    setDev(null);
    setLogOpen(false);
    log('设备已重启，连接已断开');
  }

  /* ── Firmware ────────────────────────────────────────────────── */

  async function collectFileArray() {
    if (!pkg) throw new Error('未选择固件包');
    return pkg.entries;
  }

  /* ── Flash ───────────────────────────────────────────────────── */

  async function flash() {
    const loader = loaderRef.current;
    if (!loader) return;
    setBanner(null);
    setBusy(true);
    setFlashed(false);
    setProgress({ pct: 0, text: '准备…' });
    try {
      const fileArray = await collectFileArray();
      const total = fileArray.reduce((s, f) => s + f.data.length, 0);
      log(`开始烧写 ${fileArray.length} 个分区，共 ${(total / 1024).toFixed(0)} KB`);

      const fs = pkg?.settings ?? { flash_mode: 'dio', flash_freq: '80m', flash_size: '16MB' };

      await loader.writeFlash({
        fileArray,
        flashSize: fs.flash_size ?? '16MB',
        flashMode: fs.flash_mode ?? 'dio',
        flashFreq: fs.flash_freq ?? '80m',
        eraseAll: false,
        compress: true,
        reportProgress: (fileIndex, written, totalBytes) => {
          /* cumulative progress across partitions — esptool-js's totalBytes
           * is per-file, so a raw written/totalBytes jumps 0→100% per part. */
          const grandTotal = fileArray.reduce((s, f) => s + f.data.length, 0);
          const doneBefore = fileArray.slice(0, fileIndex).reduce((s, f) => s + f.data.length, 0);
          const frac = totalBytes ? written / totalBytes : 0;
          const pct = grandTotal
            ? Math.min(100, Math.round(((doneBefore + frac * fileArray[fileIndex].data.length) / grandTotal) * 100))
            : 0;
          setProgress({
            pct,
            text: `写入分区 ${fileIndex + 1}/${fileArray.length}`,
          });
        },
        calculateMd5Hash: true,
      });
      setProgress({ pct: 100, text: '校验完成，重启设备…' });
      log('写入完成，校验通过 ✓');

      /* official esp-web-tools pattern: pull RTS low to reset first, then release via after() */
      await loader.transport.setRTS(true);
      await loader.after('hard_reset');
      log('设备已重启，应回到正常模式');
      setFlashed(true);
      setProgress({ pct: 100, text: '烧写完成 ✓' });
      await finishAfterFlash();   /* device rebooted, USB dropped — end connection state */
    } catch (err) {
      setBanner(`烧写失败: ${err?.message ?? err}`);
      log(`烧写失败: ${err?.message ?? err}`, true);
      setProgress((prev) => ({
        pct: prev?.pct ?? 0,
        error: true,
        text: '烧写失败',
      }));
    } finally {
      setBusy(false);
    }
  }

  /* ── Render ──────────────────────────────────────────────────── */

  if (!secureOk) {
    return (
      <div class="noserial">
        <span class="noserial-ico"><AlertIcon size={34} /></span>
        <h2>无法在此页面使用烧写功能</h2>
        <p>网页烧写需要加密连接（HTTPS），当前页面不满足。</p>
        <p>请打开 <b>https://zhuyu1997.github.io/esp32-mac-nano/</b> 使用本工具。</p>
      </div>
    );
  }
  if (!serialOk) {
    return (
      <div class="noserial">
        <span class="noserial-ico"><AlertIcon size={34} /></span>
        <h2>此浏览器不支持网页烧写</h2>
        <p>请用电脑上的 <b>Chrome</b>、<b>Edge</b> 或 <b>Firefox</b>（151+）浏览器打开本页。</p>
        <p>手机浏览器与 Safari 暂不支持。</p>
      </div>
    );
  }

  const step1 = conn === 'connected' ? 'done' : 'active';
  const step2 = conn === 'connected' ? (hasFw ? 'done' : 'active') : '';
  const step3 = flashed
    ? 'done'
    : conn === 'connected' && hasFw
      ? 'active'
      : '';

  /* one status line under the flash button — state-driven, no scattered hints */
  const flashNote =
    busy ? '烧写中，请勿断开 USB 线' :
    flashed ? '烧写完成，设备已重启进入正常模式' :
    conn !== 'connected' ? '' :
    !hasFw ? '请先完成步骤 2：选择固件包' :
    fwSummary();
  const flashNoteCls =
    busy ? 'flash-note warn' :
    flashed ? 'flash-note ok' :
    conn === 'connected' && hasFw ? 'flash-note info' :
    'flash-note';

  return (
    <div>
      {banner && <div id="banner" class="show">{banner}</div>}

      {/* Step 1 — connect: full-width button + one status line, same language as step 3 */}
      <div class={`step ${step1}`} id="step-connect">
        <h2><span class="step-num">{conn === 'connected' ? '✓' : '1'}</span>连接设备</h2>
        {conn === 'connecting' ? (
          <button class="flash-btn primary busy" disabled>连接中…</button>
        ) : conn === 'connected' ? (
          <button class="flash-btn" onClick={disconnect} disabled={busy}><UsbIcon size={15} />断开连接</button>
        ) : (
          <button class="flash-btn primary" onClick={connect}><UsbIcon size={15} />连接设备</button>
        )}
        {dev && (
          <div class="dev-info">
            <ChipIcon size={15} />芯片 <b>{dev.chip}</b> · Flash <b>{dev.flash}</b>
          </div>
        )}
      </div>

      {/* Step 2 — pick firmware: one clickable card, one state */}
      <div class={`step ${step2}`} id="step-firmware">
        <h2><span class="step-num">{hasFw ? '✓' : '2'}</span>选择固件</h2>
        <label
          class={`drop${pkgError ? ' err' : pkg ? ' picked' : ''}${dragOver ? ' dragover' : ''}`}
          title="选择固件包"
          onDragOver={(e) => { e.preventDefault(); setDragOver(true); }}
          onDragLeave={() => setDragOver(false)}
          onDrop={(e) => { e.preventDefault(); setDragOver(false); applyFirmwareFile(e.dataTransfer?.files?.[0]); }}
        >
          <span class="drop-ico"><FileIcon size={26} /></span>
          <span class="drop-title">
            {pkgError ? '固件包无效' : pkg ? `webflash-${pkg.version}.zip` : '选择固件包'}
          </span>
          <span class="drop-sub">
            {pkgError ? pkgError :
             pkg ? `版本 v${pkg.version} · ${pkg.entries.length} 个分区 · SHA256 校验通过 · 点击可更换` :
             '点击浏览 webflash-<版本>.zip，自动校验 SHA256 与分区表'}
          </span>
          <input type="file" accept=".zip,application/zip" onChange={onPkgChange} />
        </label>
        {pkg && (
          <div class="part-table">
            <div class="part-row part-head">
              <span>分区</span><span class="part-addr">地址</span><span class="part-size">大小</span>
            </div>
            {pkg.entries.map((f) => (
              <div class="part-row">
                <span class="part-name">{f.name}</span>
                <span class="part-addr">0x{f.address.toString(16)}</span>
                <span class="part-size">{fmtSize(f.data.length)}</span>
              </div>
            ))}
          </div>
        )}
      </div>

      {/* Step 3 — flash */}
      <div class={`step ${step3}`} id="step-flash">
        <h2><span class="step-num">{flashed ? '✓' : '3'}</span>烧写固件</h2>
        <button class="primary flash-btn" onClick={flash}
          disabled={conn !== 'connected' || !hasFw || busy}>
          {busy ? '烧写中…' : <><ZapIcon size={15} />开始烧写</>}
        </button>
        {flashNote && <div class={flashNoteCls}>{flashNote}</div>}
        {progress && (
          <>
            <div id="bar" class={progress.error ? 'error' : ''}>
              <div style={{ width: `${progress.pct}%` }}></div>
            </div>
            <div id="progress-label" class={progress.error ? 'error' : ''}>
              {progress.text}{progress.error ? '' : ` — `}
              {!progress.error && <span class="pct">{progress.pct}%</span>}
            </div>
          </>
        )}
      </div>

      {/* Log — 烧写操作日志，非设备串口日志 */}
      <details id="log-box" open={logOpen} onToggle={(e) => setLogOpen(e.target.open)}>
        <summary>
          操作日志
          <span class="log-actions">
            <button class="log-btn" onClick={onCopyLog}>{copied ? '已复制' : '复制'}</button>
            <button class="log-btn" onClick={onClearLog}>清空</button>
          </span>
        </summary>
        <div id="log" ref={logRef}></div>
      </details>
    </div>
  );
}

render(<FlashTool />, document.getElementById('app'));
