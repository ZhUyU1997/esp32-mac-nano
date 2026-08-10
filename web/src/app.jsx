import { useEffect, useRef, useState } from 'preact/hooks';
import './styles/base.css';
import { Touchpad } from './touchpad.jsx';
import { Keyboard } from './keyboard.jsx';
import { FloppyRow } from './floppy.jsx';
import { ScreenshotModal } from './screenshot.jsx';
import { sendSysKey, sendFlashMode, sendReboot, dotColor, statusText, floppyOn, floppyTitle, toastMsg, toastErr, showToast, wifiState, wifiProvisioned } from './store.js';
import { ProvisionView, SuccessView } from './wifi.jsx';

/* Fullscreen (Android Edge/Chrome support Fullscreen API; iOS doesn't → prompt to use Add to Home Screen) */
function FullscreenBtn() {
  const [fsOn, setFsOn] = useState(false);
  useEffect(() => {
    const onFs = () => setFsOn(!!document.fullscreenElement);
    document.addEventListener('fullscreenchange', onFs);
    return () => document.removeEventListener('fullscreenchange', onFs);
  }, []);
  async function toggle() {
    try {
      if (document.fullscreenElement) {
        await document.exitFullscreen();
      } else {
        await document.documentElement.requestFullscreen();
      }
    } catch (e) {
      showToast('浏览器不支持全屏，iOS 请用"添加到主屏幕"打开', true);
    }
  }
  return (
    <button className="fsbtn" id="bFull" title={fsOn ? '退出全屏' : '全屏'} aria-label="全屏" onClick={toggle}>
      <svg viewBox="0 0 16 16" width="13" height="13" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="square">
        <path d="M2.5 6.5v-4h4M13.5 6.5v-4h-4M2.5 9.5v4h4M13.5 9.5v4h-4" />
      </svg>
    </button>
  );
}

/* Status-bar dropdown menu: device actions (enter Recover/Update, etc.) */
function DeviceMenu() {
  const [open, setOpen] = useState(false);
  const ref = useRef(null);
  useEffect(() => {
    if (!open) return;
    const onDoc = (e) => {
      if (ref.current && !ref.current.contains(e.target)) setOpen(false);
    };
    document.addEventListener('pointerdown', onDoc);
    return () => document.removeEventListener('pointerdown', onDoc);
  }, [open]);
  return (
    <div className="devmenu" ref={ref}>
      <button className="devmenu-btn" title="设备" onClick={() => setOpen(!open)}>&#8943;</button>
      {open && (
        <div className="devmenu-pop">
          <button
            className="devmenu-item"
            onClick={() => {
              setOpen(false);
              showToast('正在重启设备…', false);
              sendReboot();
            }}>
            <span className="devmenu-ico">&#8635;</span>
            <span>重启设备</span>
          </button>
          <div className="devmenu-sep"></div>
          <button
            className="devmenu-item"
            onClick={() => {
              setOpen(false);
              showToast('正在重启进入恢复 / 更新模式…', false);
              sendFlashMode();
            }}>
            <span className="devmenu-ico">&#11015;</span>
            <span>进入恢复 / 更新模式</span>
          </button>
        </div>
      )}
    </div>
  );
}

function StatusBar() {
  return (
    <div className="status">
      <span id="dot" style={{ background: dotColor.value }}></span>
      <span id="st">{statusText.value}</span>
      <span className={'floppyicon' + (floppyOn.value ? ' on' : '')} id="floppyIcon" title={floppyTitle.value}></span>
      <FullscreenBtn />
      <DeviceMenu />
    </div>
  );
}

/* system keys */
const SYS = { bM: 0x10B, bBD: 0x10C, bBU: 0x10D, bVD: 0x10E, bVU: 0x10F, bVM: 0x110 };

function MenuRow() {
  return (
    <div className="panel menurow">
      <button className="btn" id="bM" title="菜单" onPointerDown={e => { e.preventDefault(); sendSysKey(SYS.bM, 1); }} onPointerUp={() => sendSysKey(SYS.bM, 0)}>&#9776;</button>
      <span className="sep"></span>
      <button className="btn" id="bBD" title="背光减" onPointerDown={e => { e.preventDefault(); sendSysKey(SYS.bBD, 1); }} onPointerUp={() => sendSysKey(SYS.bBD, 0)}>&#9728;&#8722;</button>
      <button className="btn" id="bBU" title="背光加" onPointerDown={e => { e.preventDefault(); sendSysKey(SYS.bBU, 1); }} onPointerUp={() => sendSysKey(SYS.bBU, 0)}>&#9728;&#43;</button>
      <span className="sep"></span>
      <button className="btn" id="bVD" title="音量减" onPointerDown={e => { e.preventDefault(); sendSysKey(SYS.bVD, 1); }} onPointerUp={() => sendSysKey(SYS.bVD, 0)}>&#128265;&#8722;</button>
      <button className="btn" id="bVU" title="音量加" onPointerDown={e => { e.preventDefault(); sendSysKey(SYS.bVU, 1); }} onPointerUp={() => sendSysKey(SYS.bVU, 0)}>&#128266;&#43;</button>
      <button className="btn" id="bVM" title="静音" onPointerDown={e => { e.preventDefault(); sendSysKey(SYS.bVM, 1); }} onPointerUp={() => sendSysKey(SYS.bVM, 0)}>&#128263;</button>
    </div>
  );
}

function Toast() {
  return (
    <div id="toast" className={'toast' + (toastMsg.value ? ' show' : '') + (toastErr.value ? ' err' : '')}>{toastMsg.value}</div>
  );
}

export function App() {
  /* 配网路由：纯配网界面（无遥控、无关闭）；状态未知显示加载页；
   * 只有明确 CONNECTED/RECONNECTING/AP_ONLY/OFF 才渲染遥控。 */
  const st = wifiState.value && wifiState.value.state;
  if (st === 'PROVISIONING' || st === 'CONNECTING') {
    /* 连接过程不切页面：CONNECTING 也留在配网页，状态在列表项内显示 */
    return (
      <div className="wifi-shell">
        <ProvisionView />
        <Toast />
      </div>
    );
  }
  if (st === 'CONNECTED' && wifiProvisioned.value) {
    /* 刚配网成功：显示"配网完成"提示（手机需切回路由器 WiFi 才能访问设备） */
    return (
      <div className="wifi-shell">
        <SuccessView />
        <Toast />
      </div>
    );
  }
  if (!st) {
    /* WS 未同步（页面刚打开 / 断线）：加载页，不渲染遥控也不渲染配网 */
    return (
      <div className="wifi-shell">
        <div className="wifi-center">
          <div className="wifi-spinner" />
          <div className="wifi-center-title">正在连接设备…</div>
        </div>
        <Toast />
      </div>
    );
  }

  /* capture is a function; can't store with useState (setState(fn) would call fn as updater) — use a ref */
  const captureRef = useRef(null);
  return (
    <div className="remote-shell">
      <StatusBar />
      <div className="main-col">
        <div className="panel padwrap"><Touchpad /></div>
      </div>
      <div className="landscape-side">
        <MenuRow />
        <FloppyRow onShot={() => { if (captureRef.current) captureRef.current(); }} />
        <div className="panel kb-panel"><Keyboard /></div>
      </div>
      <Toast />
      <ScreenshotModal onReady={fn => { captureRef.current = fn; }} />
    </div>
  );
}
