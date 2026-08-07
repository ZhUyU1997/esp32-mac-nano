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
    entries.push({ address: parseInt(f.address, 16), data });
    log(`  固件包: ${f.address} ${f.name} (${data.length}B)`);
  }
  return { entries, manifest };
}

/* ── Main component ─────────────────────────────────────────────── */

function FlashTool() {
  const [conn, setConn] = useState('idle');       /* idle | connecting | connected */
  const [dev, setDev] = useState(null);           /* {chip, flash} | null */
  const [banner, setBanner] = useState(null);
  const [pkg, setPkg] = useState(null);           /* {version, entries, settings} | null */
  const [pkgError, setPkgError] = useState('');
  const [logOpen, setLogOpen] = useState(false);
  const [busy, setBusy] = useState(false);
  const [flashed, setFlashed] = useState(false);   /* whether flashing completed this session */
  const [progress, setProgress] = useState(null); /* {pct, text} | null */

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

  /* title-bar status dot follows connection state: idle = red breathing / connecting = amber blink / connected = steady green */
  useEffect(() => {
    const dot = document.querySelector('.titlebar .dot');
    if (!dot) return;
    dot.className = 'dot' + (conn === 'connecting' ? ' busy' : conn === 'connected' ? ' ok' : '');
  }, [conn]);

  const terminal = useMemo(() => ({
    clean: () => { if (logRef.current) logRef.current.innerHTML = ''; },
    writeLine: (d) => log(d),
    write: (d) => log(d),
  }), [log]);

  /* firmware source: firmware package zip (manual partition feature removed for now, may return later) */
  const hasFw = !!pkg;

  const fwSummary = () => {
    if (!pkg) return '';
    return `将烧写 ${pkg.entries.length} 个分区: ` +
      pkg.entries.map((f) => `0x${f.address.toString(16)}`).join(', ');
  };

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
      log('连接设备，等待 esptool-js 握手复位…');
      await loader.main();   /* detect USJ PID → DTR/RTS handshake reset → connect */
      setConn('connected');
      setFlashed(false);
      setLogOpen(true);
      log('连接成功 — 可以加载固件');
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
    log('已断开');
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

  async function onPkgChange(e) {
    const file = e.target.files?.[0];
    if (!file) return;
    setPkgError('');
    setBanner(null);
    try {
      const { entries, manifest } = await loadFirmwarePackage(file, log);
      setPkg({ version: manifest.version, entries, settings: manifest.flash_settings ?? null });
      log(`固件包 v${manifest.version} 加载完成，SHA256 校验通过`);
    } catch (err) {
      setPkg(null);
      setPkgError(`加载失败: ${err.message}`);
      setBanner(`固件包加载失败: ${err.message}`);
      log(`固件包加载失败: ${err.message}`, true);
    }
  }

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
      log(`烧写 ${fileArray.length} 个分区: ` +
        fileArray.map((f) => `0x${f.address.toString(16)}(${f.data.length}B)`).join(', '));

      const fs = pkg?.settings ?? { flash_mode: 'dio', flash_freq: '80m', flash_size: '16MB' };

      await loader.writeFlash({
        fileArray,
        flashSize: fs.flash_size ?? '16MB',
        flashMode: fs.flash_mode ?? 'dio',
        flashFreq: fs.flash_freq ?? '80m',
        eraseAll: false,
        compress: true,
        reportProgress: (fileIndex, written, totalBytes) => {
          const pct = totalBytes ? Math.round((written / totalBytes) * 100) : 0;
          setProgress({
            pct,
            text: `写入分区 ${fileIndex + 1}/${fileArray.length}`,
          });
        },
        calculateMd5Hash: true,
      });
      setProgress({ pct: 100, text: '校验完成，重启设备…' });
      log(`写入 ${total} 字节完成，校验通过 ✓`);

      /* official esp-web-tools pattern: pull RTS low to reset first, then release via after() */
      await loader.transport.setRTS(true);
      await loader.after('hard_reset');
      log('设备已重启 — 应自动回到正常模式');
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

  const step1 = conn === 'connected' ? 'done' : 'active';
  const step2 = conn === 'connected' ? (hasFw ? 'done' : 'active') : '';
  const step3 = flashed
    ? 'done'
    : conn === 'connected' && hasFw && !busy
      ? 'active'
      : '';

  const statusText =
    conn === 'connecting' ? '连接中…' :
    conn === 'connected' ? '已连接 ✓' : '未连接';
  const statusCls =
    conn === 'connecting' ? 'status busy' :
    conn === 'connected' ? 'status ok' : 'status';

  return (
    <div>
      {banner && <div id="banner" class="show">{banner}</div>}

      {/* Step 1 */}
      <div class={`step ${step1}`} id="step-connect">
        <h2><span class="step-num">1</span>连接设备</h2>
        <div class="row">
          {conn !== 'connected' && (
            <button onClick={connect} disabled={conn === 'connecting'}>连接设备</button>
          )}
          {conn === 'connected' && <button onClick={disconnect}>断开</button>}
          <span class={statusCls}>{statusText}</span>
        </div>
        {dev && (
          <div class="dev-info">
            Chip: <b>{dev.chip}</b><br />
            Flash: <b>{dev.flash}</b>
          </div>
        )}
        <div class="hint">设备需处于 <b>Recover / Update</b> 模式（pause menu 右上角按钮，或开机按住按键）。</div>
      </div>

      {/* Step 2 */}
      <div class={`step ${step2}`} id="step-firmware">
        <h2><span class="step-num">2</span>选择固件</h2>
        <div class="row">
          <label for="pkg-file">固件包 (.zip)</label>
          <span class="filepick">
            <span class="pick-btn">选择固件包…</span>
            <span class={`pick-name ${pkg ? 'picked' : ''}`}>{pkg ? `webflash-${pkg.version}.zip` : pkgError || '未选择'}</span>
            <input type="file" accept=".zip,application/zip" id="pkg-file" onChange={onPkgChange} />
          </span>
        </div>
        {!pkg && <div class="hint">选择 <code>webflash-&lt;版本&gt;.zip</code>（自动校验 SHA256 + 填好分区）。</div>}
        {pkg && (
          <div class="hint">已加载固件包 v{pkg.version}（{pkg.entries.length} 个分区，SHA256 ✓）</div>
        )}
      </div>

      {/* Step 3 */}
      <div class={`step ${step3}`} id="step-flash">
        <h2><span class="step-num">3</span>烧写</h2>
        <div class="row">
          <button class="primary" onClick={flash}
            disabled={!loaderRef.current || !hasFw || busy}>{busy ? '烧写中…' : '开始烧写'}</button>
          <span class="hint" style={{ margin: 0 }}>
            {fwSummary() ||
              (conn === 'connected' ? '请先选择固件包' : '请先连接设备')}
          </span>
        </div>
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

      {/* Log */}
      <details id="log-box" open={logOpen} onToggle={(e) => setLogOpen(e.target.open)}>
        <summary>日志</summary>
        <div id="log" ref={logRef}></div>
      </details>
    </div>
  );
}

render(<FlashTool />, document.getElementById('app'));
