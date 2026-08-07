import { useEffect, useRef, useState } from 'preact/hooks';
import Cropper from 'cropperjs';
import './styles/screenshot.css';
import { shotBusy, showToast } from './store.js';

const SRC_W = 640, SRC_H = 480;
const RATIOS = [
  ['free', '自由'], ['screen', '全屏'], ['1', '1:1'], ['0.75', '3:4'],
  ['1.3333', '4:3'], ['0.5625', '9:16'], ['1.7778', '16:9'],
];

/* ── Screenshot modal: phone wallpaper creation (free crop + scale/rotate, exports 1080x2400 canvas) ──
 * Cropper is an imperative DOM library: the modal element stays in the DOM (shown/hidden), the img element is never rebuilt. */
export function ScreenshotModal({ onReady }) {
  const popRef = useRef(null);
  const imgRef = useRef(null);
  const viewRef = useRef(null);
  const wallRef = useRef(null);
  const dialRef = useRef(null);
  const bandRef = useRef(null);
  const labelRef = useRef(null);
  const resetRef = useRef(null);
  const [ratioSel, setRatioSel] = useState('free');
  /* Imperative state (Cropper instance etc.), not part of rendering */
  const st = useRef({ srcUrl: null, srcCanvas: null, cropper: null, initZoom: 1, centering: false });

  async function capture() {
    if (shotBusy.value) return;
    shotBusy.value = true;
    try {
      const ac = new AbortController();
      const t = setTimeout(() => ac.abort(), 8000);
      const resp = await fetch('/api/screenshot', { signal: ac.signal });
      clearTimeout(t);
      if (!resp.ok) { showToast('截屏失败，请重试', true); return; }
      const buf = new Uint8Array(await resp.arrayBuffer());
      /* 1-bit → 640x480 grayscale canvas → dataURL */
      const c = document.createElement('canvas');
      c.width = SRC_W; c.height = SRC_H;
      const ctx = c.getContext('2d');
      const img = ctx.createImageData(SRC_W, SRC_H);
      const d = img.data;
      for (let i = 0; i < SRC_W * SRC_H; i++) {
        const bit = (buf[i >> 3] >> (7 - (i & 7))) & 1;
        const v = bit ? 0 : 255;
        const p = i * 4;
        d[p] = d[p + 1] = d[p + 2] = v; d[p + 3] = 255;
      }
      ctx.putImageData(img, 0, 0);
      st.current.srcCanvas = c;                  /* original 640x480 screenshot canvas (inverse-mapped sampling at download) */
      st.current.srcUrl = c.toDataURL('image/png');
      /* Load the image first, then show the modal and create Cropper */
      imgRef.current.src = st.current.srcUrl;
      await new Promise(r => {
        if (imgRef.current.complete) r();
        else imgRef.current.onload = r;
      });
      popRef.current.hidden = false;   /* show first — Cropper needs visible dimensions */
      /* Reset ratio buttons (Cropper is rebuilt as free-ratio each time, highlight syncs back to "free") */
      setRatioSel('free');
      await seg();
    } catch (e) {
      if (e && e.name === 'AbortError') showToast('截屏超时，请重试', true);
      else showToast('截屏失败，请检查连接后重试', true);
    } finally {
      shotBusy.value = false;
    }
  }

  async function seg() {
    wallRef.current.style.display = 'flex';
    /* Rebuild Cropper: wait for the image to load before creating */
    imgRef.current.style.display = 'block';
    if (st.current.cropper) { st.current.cropper.destroy(); st.current.cropper = null; }
    await createCropper();
  }

  async function createCropper() {
    if (st.current.cropper) return;
    try { await imgRef.current.decode(); } catch (e) { /* already loaded, skip */ }
    await new Promise(r => requestAnimationFrame(() => r()));
    if (st.current.cropper) return;
    try {
      st.current.cropper = new Cropper(imgRef.current, {
        aspectRatio: NaN,          /* free ratio — crop box freely adjustable */
        viewMode: 0,               /* unrestricted — can zoom out */
        autoCropArea: 1,           /* default: crop full image */
        dragMode: 'move',          /* drag image = pan */
        zoomable: true,
        rotatable: true,
        scalable: false,
        cropBoxMovable: false,     /* box not dragged alone — dragging moves the image (box follows) */
        cropBoxResizable: true,
        cropend: () => {
          /* on cropend: center the crop box — move the image so the box center hits the container center,
           * the box's position on the image (crop area) stays unchanged */
          if (st.current.centering) return;
          st.current.centering = true;
          const data = st.current.cropper.getData();      /* current crop area (image coords) */
          const con = st.current.cropper.getContainerData();
          const can = st.current.cropper.getCanvasData();
          const box = st.current.cropper.getCropBoxData();
          const boxCx = box.left + box.width / 2;   /* box center (container coords) */
          const boxCy = box.top + box.height / 2;
          const scale = can.width / can.naturalWidth;   /* image display scale */
          const dx = (con.width / 2 - boxCx) / scale;
          const dy = (con.height / 2 - boxCy) / scale;
          st.current.cropper.setCanvasData({
            left: can.left + dx * scale,
            top: can.top + dy * scale,
          });
          /* Restore the crop area (image coords) — image moved but crop area unchanged */
          st.current.cropper.setData({
            x: data.x, y: data.y,
            width: data.width, height: data.height,
            rotate: data.rotate || 0,
          });
          st.current.centering = false;
        },
        toggleDragModeOnDblclick: false,
        ready: () => {
          try {
            /* initial zoom-out 25% — shrink the image; align the box with setData (image coords) */
            st.current.cropper.zoom(-0.25);
            st.current.cropper.setData({
              x: 0, y: 0,
              width: st.current.cropper.getImageData().naturalWidth,
              height: st.current.cropper.getImageData().naturalHeight,
            });
            const id = st.current.cropper.getImageData();
            st.current.initZoom = id.width / id.naturalWidth;
          } catch (e) { /* ignore — init failure must not block */ }
        },
      });
    } catch (e) { /* ignore */ }
  }

  /* HD download: inverse-mapped hard sampling + edge pixels weighted by distance (coverage approximation, FreeType-style analytic AA) */
  function downloadHD() {
    if (!st.current.srcCanvas || !st.current.cropper) return;
    const src = st.current.srcCanvas;
    const sw = src.width, sh = src.height;
    const sdata = src.getContext('2d').getImageData(0, 0, sw, sh).data;
    const data = st.current.cropper.getData();
    const rot = (data.rotate || 0) * Math.PI / 180;
    const cw = data.width, ch = data.height;
    /* rotated image size (Cropper natural size updates with rotation) — getData coords are based on it */
    const canData = st.current.cropper.getCanvasData();
    const rotW = canData.naturalWidth || sw, rotH = canData.naturalHeight || sh;
    /* rotation center = image center (unrotated image coords) */
    const icx = sw / 2, icy = sh / 2;
    /* output size: both dimensions fit the screen (image ≥ screen, no content crop), keep crop ratio, long edge capped at 4K */
    const dpr = window.devicePixelRatio || 1;
    const scrW = Math.round((window.screen.width || 1080) * dpr);
    const scrH = Math.round((window.screen.height || 2400) * dpr);
    /* take the larger of width/height fit → both dimensions ≥ screen */
    const scale = Math.max(scrW / cw, scrH / ch);
    let dw = Math.round(cw * scale), dh = Math.round(ch * scale);
    /* long edge cap 4K: uniform downscale */
    const cap = 3840;
    if (dw > cap || dh > cap) {
      const s = Math.min(cap / dw, cap / dh);
      dw = Math.round(dw * s); dh = Math.round(dh * s);
    }
    const scx = dw / cw, scy = dh / ch;   /* output px → rotated image coords */
    const cos = Math.cos(rot), sin = Math.sin(rot);   /* inverse rotation (around image center) */
    /* projected half-width of an output pixel (rotated diamond) in source space — for edge detection */
    const hw = (Math.abs(cos) / scx + Math.abs(sin) / scy) / 2;
    const hh = (Math.abs(cos) / scy + Math.abs(sin) / scx) / 2;
    const out = document.createElement('canvas');
    out.width = dw; out.height = dh;
    const octx = out.getContext('2d');
    const idata = octx.createImageData(dw, dh);
    const o = idata.data;
    /* incremental stepping: inverse map is linear, fixed step within/between rows
     * output px → position in box (rotated image coords) → relative to rotated image center → inverse rotate around center → unrotated source coords
     * note: canvas positive angle = clockwise; inverse map uses counter-clockwise (cos same sign, sin negated) */
    const dSx = cos / scx, dSy = -sin / scx;      /* within row (ox += 1) */
    const dVy = sin / scy, dVy2 = cos / scy;      /* between rows (oy += 1) */
    const px0 = data.x + 0.5 / scx - rotW / 2, py0 = data.y + 0.5 / scy - rotH / 2;
    let sx = icx + px0 * cos + py0 * sin, sy = icy - px0 * sin + py0 * cos;
    for (let oy = 0; oy < dh; oy++) {
      let lx = sx, ly = sy;
      let di = oy * dw * 4;
      for (let ox = 0; ox < dw; ox++) {
        const ix = Math.floor(lx), iy = Math.floor(ly);
        const fx = lx - ix, fy = ly - iy;   /* distance from top-left grid line */
        /* edge detection: distance from center to grid line < projected half-width (crosses boundary) */
        const hEdge = fx < hw || fx > 1 - hw;
        const vEdge = fy < hh || fy > 1 - hh;
        if (hEdge || vEdge) {
          /* edge: coverage split — 0.5 ± d/W (d = center-to-boundary distance, W = projected pixel width) */
          if (ix >= 0 && ix < sw && iy >= 0 && iy < sh) {
            /* horizontal: two source pixels to blend and their weights */
            let xL = ix, xR = ix, wL = 1, wR = 0;
            if (fx < hw)      { xL = ix - 1; xR = ix; wL = 0.5 - fx / (2 * hw); wR = 1 - wL; }
            else if (fx > 1 - hw) { xL = ix; xR = ix + 1; wL = 0.5 + (1 - fx) / (2 * hw); wR = 1 - wL; }
            /* vertical */
            let yT = iy, yB = iy, wT = 1, wB = 0;
            if (fy < hh)      { yT = iy - 1; yB = iy; wT = 0.5 - fy / (2 * hh); wB = 1 - wT; }
            else if (fy > 1 - hh) { yT = iy; yB = iy + 1; wT = 0.5 + (1 - fy) / (2 * hh); wB = 1 - wT; }
            /* out-of-bounds neighbors treated as black (off-screen), no clamping — otherwise weights would be misassigned to edge pixels */
            const val = (x, y) => (x >= 0 && x < sw && y >= 0 && y < sh) ? sdata[(y * sw + x) * 4] : 0;
            const v = val(xL, yT) * wL * wT + val(xR, yT) * wR * wT
                    + val(xL, yB) * wL * wB + val(xR, yB) * wR * wB;
            o[di] = o[di + 1] = o[di + 2] = Math.round(v); o[di + 3] = 255;
          } else {
            o[di] = o[di + 1] = o[di + 2] = 0; o[di + 3] = 255;   /* out of bounds = black */
          }
        } else {
          /* interior: sample source pixel directly (pure 0/255) — bounds check required, otherwise the 1-D index reads into other rows */
          if (ix >= 0 && ix < sw && iy >= 0 && iy < sh) {
            const si = (iy * sw + ix) * 4;
            o[di] = sdata[si]; o[di + 1] = sdata[si + 1]; o[di + 2] = sdata[si + 2]; o[di + 3] = 255;
          } else {
            o[di] = o[di + 1] = o[di + 2] = 0; o[di + 3] = 255;   /* out of bounds = black */
          }
        }
        lx += dSx; ly += dSy;
        di += 4;
      }
      sx += dVy; sy += dVy2;
    }
    octx.putImageData(idata, 0, 0);
    const a = document.createElement('a');
    a.href = out.toDataURL('image/png');
    a.download = 'mac-wallpaper-' + dw + 'x' + dh + '.png';
    a.click();
  }

  function download() {
    if (!st.current.srcUrl || !st.current.cropper) return;
    /* original resolution: crop area 1:1, no upscale */
    const c = st.current.cropper.getCroppedCanvas();
    const a = document.createElement('a');
    a.href = c.toDataURL('image/png');
    a.download = 'mac-' + c.width + 'x' + c.height + '.png';
    a.click();
  }

  function close() { popRef.current.hidden = true; }

  /* ratio pills: click to set Cropper aspect ratio — keep current crop box area (contain-adjust, don't reset to edges) */
  function onRatio(r) {
    if (!st.current.cropper) return;
    setRatioSel(r);
    const d = st.current.cropper.getData();
    const cx = d.x + d.width / 2, cy = d.y + d.height / 2;
    const lock = r === 'free' ? NaN : (r === 'screen' ? window.screen.width / window.screen.height : parseFloat(r));
    /* target box (image coords): free = keep; otherwise convert by current area to target ratio, clamp only beyond image bounds */
    let target;
    if (r === 'free') {
      target = { x: d.x, y: d.y, width: d.width, height: d.height };
    } else {
      const area = d.width * d.height;
      let nw = Math.sqrt(area * lock), nh = nw / lock;   /* lock is w/h, area = nw²×lock */
      if (nw > 640) { nw = 640; nh = nw / lock; }
      if (nh > 480) { nh = 480; nw = nh * lock; }
      target = { x: cx - nw / 2, y: cy - nh / 2, width: nw, height: nh };
    }
    /* lock ratio first (setAspectRatio resets the box), then apply target box */
    st.current.cropper.setAspectRatio(lock);
    st.current.cropper.setData(target);
    /* visual centering: move image so box center hits container center, then restore crop area (target values) */
    const con = st.current.cropper.getContainerData();
    const can = st.current.cropper.getCanvasData();
    const box = st.current.cropper.getCropBoxData();
    const scale = can.width / can.naturalWidth;
    const dx = (con.width / 2 - (box.left + box.width / 2)) / scale;
    const dy = (con.height / 2 - (box.top + box.height / 2)) / scale;
    st.current.cropper.setCanvasData({ left: can.left + dx * scale, top: can.top + dy * scale });
    st.current.cropper.setData(target);
  }

  /* one-time init: tick band + dial/inner-drag gestures + Escape (imperative DOM events) */
  useEffect(() => {
    const dial = dialRef.current;
    const band = bandRef.current;
    const label = labelRef.current;
    const resetBtn = resetRef.current;
    const view = viewRef.current;
    /* linear tick band: band moves with angle, center indicator fixed (simple version)
     * ticks: ±180°, one every 8.5° (10° densified 15%)
     * TICK_SPAN = span of ±180°; band physical width = span + container width (always covers container after translate) */
    const TICK_SPAN = 400, TICK_STEP = 8.5, BAND_W = TICK_SPAN + 280, BAND_CX = BAND_W / 2;
    band.style.width = BAND_W + 'px';
    for (let a = -180; a <= 180; a += TICK_STEP) {
      const t = document.createElement('div');
      t.className = 'wd-tick';
      t.style.left = (BAND_CX + a / 360 * TICK_SPAN) + 'px';
      band.appendChild(t);
    }
    /* while dragging: only update angle readout (no mutual-exclusion switching) */
    const updateAngle = () => {
      const rot = (st.current.cropper ? st.current.cropper.getData().rotate || 0 : 0) % 360;
      band.style.transform = 'translateX(' + (rot / 360 * TICK_SPAN) + 'px)';
      label.textContent = Math.round(rot) + '°';
    };
    /* after movement stops: mutual-exclusion toggle (0° readout / non-0° reset button) */
    const updateMutual = () => {
      const rot = (st.current.cropper ? st.current.cropper.getData().rotate || 0 : 0) % 360;
      const isZero = Math.round(rot) === 0;
      label.style.display = isZero ? 'block' : 'none';
      resetBtn.style.display = isZero ? 'none' : 'block';
    };
    /* current angle (normalized to -180..180) */
    const currentRot = () => {
      const r = (st.current.cropper ? st.current.cropper.getData().rotate || 0 : 0) % 360;
      return r > 180 ? r - 360 : r < -180 ? r + 360 : r;
    };
    let dialDrag = false, dialLast = 0;
    const onDialDown = e => {
      if (!st.current.cropper) return;
      e.preventDefault(); dial.setPointerCapture(e.pointerId);
      dialDrag = true; dialLast = e.clientX;
      /* drag start: restore readout display (may have been hidden by reset-button exclusivity after last release) */
      label.style.display = 'block';
      resetBtn.style.display = 'none';
    };
    const onDialMove = e => {
      if (!dialDrag || !st.current.cropper) return;
      const dx = e.clientX - dialLast;
      dialLast = e.clientX;
      /* drag 2px ≈ 1°, clamped to ±180° (ticks must not overflow 180) */
      const next = Math.max(-180, Math.min(180, currentRot() + dx / 2));
      st.current.cropper.rotateTo(next);
      updateAngle();
    };
    const onDialUp = () => { dialDrag = false; updateMutual(); };
    dial.addEventListener('pointerdown', onDialDown);
    dial.addEventListener('pointermove', onDialMove);
    dial.addEventListener('pointerup', onDialUp);

    /* reset: pointerdown + stopPropagation (wallDial's pointerdown preventDefault swallows click) */
    const onReset = e => {
      e.stopPropagation();
      e.preventDefault();
      if (!st.current.cropper) return;
      /* reset: cropper.reset (position centered / rotation zeroed / original scale) → then apply zoom-out margin + box alignment */
      st.current.cropper.reset();
      st.current.cropper.zoomTo(st.current.initZoom || 1);
      st.current.cropper.setData({
        x: 0, y: 0,
        width: st.current.cropper.getImageData().naturalWidth,
        height: st.current.cropper.getImageData().naturalHeight,
      });
      updateAngle(); updateMutual();
    };
    resetBtn.addEventListener('pointerdown', onReset);

    /* inner-box drag = move image (intercept at capture phase — when crop box = full image there's nowhere else to drag, inner area must move the image too) */
    let innerDrag = false, innerX = 0, innerY = 0;
    const HANDLE = 16;   /* Cropper handle zone (px) — not intercepted, so resize still works */
    const onViewDown = e => {
      if (!st.current.cropper) return;
      /* touch: pass through entirely to Cropper native (one finger = move, two = zoom) — intercepting breaks pinch */
      if (e.pointerType === 'touch') return;
      const box = st.current.cropper.getCropBoxData();
      const r = view.getBoundingClientRect();
      const x = e.clientX - r.left, y = e.clientY - r.top;
      const inBox = x >= box.left && x <= box.left + box.width && y >= box.top && y <= box.top + box.height;
      const nearEdge = x < box.left + HANDLE || x > box.left + box.width - HANDLE ||
                       y < box.top + HANDLE || y > box.top + box.height - HANDLE;
      if (inBox && !nearEdge) {   /* inside box, not handle zone — move image */
        e.stopPropagation();
        innerDrag = true; innerX = e.clientX; innerY = e.clientY;
      }
    };
    const onWinMove = e => {
      if (!innerDrag || !st.current.cropper) return;
      const can = st.current.cropper.getCanvasData();
      st.current.cropper.setCanvasData({
        left: can.left + (e.clientX - innerX),
        top: can.top + (e.clientY - innerY),
      });
      innerX = e.clientX; innerY = e.clientY;
    };
    const onWinUp = () => { innerDrag = false; };
    view.addEventListener('pointerdown', onViewDown, true);
    window.addEventListener('pointermove', onWinMove);
    window.addEventListener('pointerup', onWinUp);
    updateAngle(); updateMutual();

    const onKey = e => { if (e.key === 'Escape') close(); };
    document.addEventListener('keydown', onKey);

    return () => {
      dial.removeEventListener('pointerdown', onDialDown);
      dial.removeEventListener('pointermove', onDialMove);
      dial.removeEventListener('pointerup', onDialUp);
      resetBtn.removeEventListener('pointerdown', onReset);
      view.removeEventListener('pointerdown', onViewDown, true);
      window.removeEventListener('pointermove', onWinMove);
      window.removeEventListener('pointerup', onWinUp);
      document.removeEventListener('keydown', onKey);
      if (st.current.cropper) { st.current.cropper.destroy(); st.current.cropper = null; }
    };
  }, []);

  /* register capture with App (screenshot button lives in FloppyRow) — pass the function directly,
   * don't wrap in an arrow function (App's setState(fn) would call fn as an updater) */
  useEffect(() => {
    onReady(capture);
  }, []);

  return (
    <div className="shotpop" id="shotPop" ref={popRef} hidden
      onClick={e => { if (e.target === popRef.current) close(); }}>
      <div className="shotbox">
        <div className="shotview" id="shotView" ref={viewRef}>
          <img id="shotImg" ref={imgRef} alt="" />
        </div>
        <div className="wallctrl" id="wallCtrl" ref={wallRef}>
          <div className="ratio-row" id="ratioRow">
            {RATIOS.map(([r, label]) => (
              <button key={r} className={'ratio-btn' + (ratioSel === r ? ' on' : '')} data-ratio={r}
                onClick={() => onRatio(r)}>{label}</button>
            ))}
          </div>
          <div className="walldial" id="wallDial" ref={dialRef} title="旋转">
            <div className="wd-band" id="wdBand" ref={bandRef}></div>
            <div className="wd-indicator"></div>
            <div className="wd-label" id="wdLabel" ref={labelRef}>0°</div>
            <button className="wd-reset" id="wallReset" ref={resetRef}>还原</button>
          </div>
        </div>
        <div className="shotfoot">
          <button className="shotdl" id="shotDl" title="原始分辨率，不放大" onClick={download}>下载图片</button>
          <button className="shotdl" id="dlHD" title="屏幕物理分辨率宽度，高度随裁切比例——还原原始像素方块" onClick={downloadHD}>下载高清壁纸</button>
          <button className="shotclose" id="shotClose" onClick={close}>关闭</button>
        </div>
      </div>
    </div>
  );
}
