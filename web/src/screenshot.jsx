import { useEffect, useRef, useState } from 'preact/hooks';
import Cropper from 'cropperjs';
import './styles/screenshot.css';
import { shotBusy, showToast } from './store.js';

const SRC_W = 640, SRC_H = 480;
const RATIOS = [
  ['free', '自由'], ['screen', '全屏'], ['1', '1:1'], ['0.75', '3:4'],
  ['1.3333', '4:3'], ['0.5625', '9:16'], ['1.7778', '16:9'],
];

/* ── 截屏弹窗：手机壁纸制作（自由裁切 + 缩放/旋转，导出 1080x2400 画布）──
 * Cropper 是命令式 DOM 库：弹窗元素常驻 DOM（hidden 显隐），img 元素永不重建。 */
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
  /* 命令式状态（Cropper 实例等），不参与渲染 */
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
      /* 1-bit → 640x480 灰度 canvas → dataURL */
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
      st.current.srcCanvas = c;                  /* 原始 640x480 截图画布（下载时直接逆映射采样） */
      st.current.srcUrl = c.toDataURL('image/png');
      /* 先加载图片，再显示弹窗创建 Cropper */
      imgRef.current.src = st.current.srcUrl;
      await new Promise(r => {
        if (imgRef.current.complete) r();
        else imgRef.current.onload = r;
      });
      popRef.current.hidden = false;   /* 先显示——Cropper 需要可见尺寸 */
      /* 重置比例按钮（Cropper 每次重建为自由比例，高亮同步回"自由"） */
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
    /* 重建 Cropper：等图片加载完成再创建 */
    imgRef.current.style.display = 'block';
    if (st.current.cropper) { st.current.cropper.destroy(); st.current.cropper = null; }
    await createCropper();
  }

  async function createCropper() {
    if (st.current.cropper) return;
    try { await imgRef.current.decode(); } catch (e) { /* 已加载则跳过 */ }
    await new Promise(r => requestAnimationFrame(() => r()));
    if (st.current.cropper) return;
    try {
      st.current.cropper = new Cropper(imgRef.current, {
        aspectRatio: NaN,          /* 比例自由——裁剪框可任意调整 */
        viewMode: 0,               /* 无限制——可缩小 */
        autoCropArea: 1,           /* 默认全图裁剪 */
        dragMode: 'move',          /* 拖图片 = 平移图片 */
        zoomable: true,
        rotatable: true,
        scalable: false,
        cropBoxMovable: false,     /* 框不单独拖——拖动统一移图（图+框同步） */
        cropBoxResizable: true,
        cropend: () => {
          /* resize 结束：取景区（裁剪框）居中——图片移动使框中心到容器中心，
           * 框在图片上的位置（取景区）不变 */
          if (st.current.centering) return;
          st.current.centering = true;
          const data = st.current.cropper.getData();      /* 当前取景区（图片坐标） */
          const con = st.current.cropper.getContainerData();
          const can = st.current.cropper.getCanvasData();
          const box = st.current.cropper.getCropBoxData();
          const boxCx = box.left + box.width / 2;   /* 框中心（容器坐标） */
          const boxCy = box.top + box.height / 2;
          const scale = can.width / can.naturalWidth;   /* 图片显示缩放 */
          const dx = (con.width / 2 - boxCx) / scale;
          const dy = (con.height / 2 - boxCy) / scale;
          st.current.cropper.setCanvasData({
            left: can.left + dx * scale,
            top: can.top + dy * scale,
          });
          /* 恢复取景区（图片坐标）——图片移了但取景区不变 */
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
            /* 初始整体缩小 25%——图片缩小；框用 setData（图片坐标）对齐 */
            st.current.cropper.zoom(-0.25);
            st.current.cropper.setData({
              x: 0, y: 0,
              width: st.current.cropper.getImageData().naturalWidth,
              height: st.current.cropper.getImageData().naturalHeight,
            });
            const id = st.current.cropper.getImageData();
            st.current.initZoom = id.width / id.naturalWidth;
          } catch (e) { /* 忽略——初始化失败不阻塞 */ }
        },
      });
    } catch (e) { /* 忽略 */ }
  }

  /* 高清下载：逆映射硬采样 + 边缘像素按距离分配（覆盖率近似，FreeType 风格解析 AA） */
  function downloadHD() {
    if (!st.current.srcCanvas || !st.current.cropper) return;
    const src = st.current.srcCanvas;
    const sw = src.width, sh = src.height;
    const sdata = src.getContext('2d').getImageData(0, 0, sw, sh).data;
    const data = st.current.cropper.getData();
    const rot = (data.rotate || 0) * Math.PI / 180;
    const cw = data.width, ch = data.height;
    /* 旋转后图片尺寸（Cropper 自然尺寸随旋转更新）——getData 坐标基于它 */
    const canData = st.current.cropper.getCanvasData();
    const rotW = canData.naturalWidth || sw, rotH = canData.naturalHeight || sh;
    /* 旋转中心 = 图片中心（未旋转图片坐标） */
    const icx = sw / 2, icy = sh / 2;
    /* 输出尺寸：宽高都适配屏幕（保证图片 ≥ 屏幕，不裁内容），保持裁切比例，长边上限 4K */
    const dpr = window.devicePixelRatio || 1;
    const scrW = Math.round((window.screen.width || 1080) * dpr);
    const scrH = Math.round((window.screen.height || 2400) * dpr);
    /* 取宽/高两种适配的较大缩放 → 两方向都 ≥ 屏幕 */
    const scale = Math.max(scrW / cw, scrH / ch);
    let dw = Math.round(cw * scale), dh = Math.round(ch * scale);
    /* 长边上限 4K：等比缩小 */
    const cap = 3840;
    if (dw > cap || dh > cap) {
      const s = Math.min(cap / dw, cap / dh);
      dw = Math.round(dw * s); dh = Math.round(dh * s);
    }
    const scx = dw / cw, scy = dh / ch;   /* 输出像素 → 旋转后图片坐标 */
    const cos = Math.cos(rot), sin = Math.sin(rot);   /* 逆旋转（绕图片中心） */
    /* 输出像素（旋转菱形）在源空间的投影半宽——用于边缘检测 */
    const hw = (Math.abs(cos) / scx + Math.abs(sin) / scy) / 2;
    const hh = (Math.abs(cos) / scy + Math.abs(sin) / scx) / 2;
    const out = document.createElement('canvas');
    out.width = dw; out.height = dh;
    const octx = out.getContext('2d');
    const idata = octx.createImageData(dw, dh);
    const o = idata.data;
    /* 增量步进：逆映射线性，行内/行间步长固定
     * 输出像素 → 框内位置（旋转后图片坐标）→ 相对旋转后图片中心 → 绕图片中心逆旋转 → 未旋转源坐标
     * 注意：canvas 正角=顺时针，逆映射用逆时针变换（cos 同号，sin 反号） */
    const dSx = cos / scx, dSy = -sin / scx;      /* 行内（ox 增 1） */
    const dVy = sin / scy, dVy2 = cos / scy;      /* 行间（oy 增 1） */
    const px0 = data.x + 0.5 / scx - rotW / 2, py0 = data.y + 0.5 / scy - rotH / 2;
    let sx = icx + px0 * cos + py0 * sin, sy = icy - px0 * sin + py0 * cos;
    for (let oy = 0; oy < dh; oy++) {
      let lx = sx, ly = sy;
      let di = oy * dw * 4;
      for (let ox = 0; ox < dw; ox++) {
        const ix = Math.floor(lx), iy = Math.floor(ly);
        const fx = lx - ix, fy = ly - iy;   /* 距左上网格线距离 */
        /* 边缘检测：中心到网格线距离 < 像素投影半宽（跨边界） */
        const hEdge = fx < hw || fx > 1 - hw;
        const vEdge = fy < hh || fy > 1 - hh;
        if (hEdge || vEdge) {
          /* 边缘：覆盖率分配——0.5 ± d/W（d=中心到边界距离，W=像素投影宽度） */
          if (ix >= 0 && ix < sw && iy >= 0 && iy < sh) {
            /* 水平：参与混合的两个源像素及其权重 */
            let xL = ix, xR = ix, wL = 1, wR = 0;
            if (fx < hw)      { xL = ix - 1; xR = ix; wL = 0.5 - fx / (2 * hw); wR = 1 - wL; }
            else if (fx > 1 - hw) { xL = ix; xR = ix + 1; wL = 0.5 + (1 - fx) / (2 * hw); wR = 1 - wL; }
            /* 垂直 */
            let yT = iy, yB = iy, wT = 1, wB = 0;
            if (fy < hh)      { yT = iy - 1; yB = iy; wT = 0.5 - fy / (2 * hh); wB = 1 - wT; }
            else if (fy > 1 - hh) { yT = iy; yB = iy + 1; wT = 0.5 + (1 - fy) / (2 * hh); wB = 1 - wT; }
            /* 越界邻居视为黑色（屏幕外），不 clamp —— 否则权重会错误算给边缘像素 */
            const val = (x, y) => (x >= 0 && x < sw && y >= 0 && y < sh) ? sdata[(y * sw + x) * 4] : 0;
            const v = val(xL, yT) * wL * wT + val(xR, yT) * wR * wT
                    + val(xL, yB) * wL * wB + val(xR, yB) * wR * wB;
            o[di] = o[di + 1] = o[di + 2] = Math.round(v); o[di + 3] = 255;
          } else {
            o[di] = o[di + 1] = o[di + 2] = 0; o[di + 3] = 255;   /* 越界 = 黑 */
          }
        } else {
          /* 内部：直接取源像素（纯 0/255）——必须边界检查，否则一维索引会读到其他行数据 */
          if (ix >= 0 && ix < sw && iy >= 0 && iy < sh) {
            const si = (iy * sw + ix) * 4;
            o[di] = sdata[si]; o[di + 1] = sdata[si + 1]; o[di + 2] = sdata[si + 2]; o[di + 3] = 255;
          } else {
            o[di] = o[di + 1] = o[di + 2] = 0; o[di + 3] = 255;   /* 越界 = 黑 */
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
    /* 原始分辨率：裁切区 1:1，不放大 */
    const c = st.current.cropper.getCroppedCanvas();
    const a = document.createElement('a');
    a.href = c.toDataURL('image/png');
    a.download = 'mac-' + c.width + 'x' + c.height + '.png';
    a.click();
  }

  function close() { popRef.current.hidden = true; }

  /* 比例胶囊：点击设置 Cropper 裁剪比例——保持当前裁剪框区域（contain 调整，不重置到贴边） */
  function onRatio(r) {
    if (!st.current.cropper) return;
    setRatioSel(r);
    const d = st.current.cropper.getData();
    const cx = d.x + d.width / 2, cy = d.y + d.height / 2;
    const lock = r === 'free' ? NaN : (r === 'screen' ? window.screen.width / window.screen.height : parseFloat(r));
    /* 目标框（图片坐标）：自由=保持；否则按当前面积换算目标比例，仅超图片边界才 clamp */
    let target;
    if (r === 'free') {
      target = { x: d.x, y: d.y, width: d.width, height: d.height };
    } else {
      const area = d.width * d.height;
      let nw = Math.sqrt(area * lock), nh = nw / lock;   /* lock 是 w/h，面积 = nw²×lock */
      if (nw > 640) { nw = 640; nh = nw / lock; }
      if (nh > 480) { nh = 480; nw = nh * lock; }
      target = { x: cx - nw / 2, y: cy - nh / 2, width: nw, height: nh };
    }
    /* 先锁定比例（setAspectRatio 会重置框），再应用目标框 */
    st.current.cropper.setAspectRatio(lock);
    st.current.cropper.setData(target);
    /* 视觉居中：移动图片使框中心到容器中心，然后恢复取景区（目标值） */
    const con = st.current.cropper.getContainerData();
    const can = st.current.cropper.getCanvasData();
    const box = st.current.cropper.getCropBoxData();
    const scale = can.width / can.naturalWidth;
    const dx = (con.width / 2 - (box.left + box.width / 2)) / scale;
    const dy = (con.height / 2 - (box.top + box.height / 2)) / scale;
    st.current.cropper.setCanvasData({ left: can.left + dx * scale, top: can.top + dy * scale });
    st.current.cropper.setData(target);
  }

  /* 一次性初始化：刻度带 + 拨盘/框内拖动手势 + Escape（命令式 DOM 事件） */
  useEffect(() => {
    const dial = dialRef.current;
    const band = bandRef.current;
    const label = labelRef.current;
    const resetBtn = resetRef.current;
    const view = viewRef.current;
    /* 线性刻度条：刻度带随角度移动，中心指示器固定（简洁版）
     * 生成刻度：±180°，每 8.5° 一条（10° 加密 15%）
     * TICK_SPAN = 刻度 ±180° 的跨度；band 物理宽 = 跨度 + 容器宽（平移后始终覆盖容器） */
    const TICK_SPAN = 400, TICK_STEP = 8.5, BAND_W = TICK_SPAN + 280, BAND_CX = BAND_W / 2;
    band.style.width = BAND_W + 'px';
    for (let a = -180; a <= 180; a += TICK_STEP) {
      const t = document.createElement('div');
      t.className = 'wd-tick';
      t.style.left = (BAND_CX + a / 360 * TICK_SPAN) + 'px';
      band.appendChild(t);
    }
    /* 拖动中：只更新角度读数（不切换互斥） */
    const updateAngle = () => {
      const rot = (st.current.cropper ? st.current.cropper.getData().rotate || 0 : 0) % 360;
      band.style.transform = 'translateX(' + (rot / 360 * TICK_SPAN) + 'px)';
      label.textContent = Math.round(rot) + '°';
    };
    /* 停止移动后：互斥切换（0° 读数 / 非 0° 还原按钮） */
    const updateMutual = () => {
      const rot = (st.current.cropper ? st.current.cropper.getData().rotate || 0 : 0) % 360;
      const isZero = Math.round(rot) === 0;
      label.style.display = isZero ? 'block' : 'none';
      resetBtn.style.display = isZero ? 'none' : 'block';
    };
    /* 当前角度（归一化到 -180..180） */
    const currentRot = () => {
      const r = (st.current.cropper ? st.current.cropper.getData().rotate || 0 : 0) % 360;
      return r > 180 ? r - 360 : r < -180 ? r + 360 : r;
    };
    let dialDrag = false, dialLast = 0;
    const onDialDown = e => {
      if (!st.current.cropper) return;
      e.preventDefault(); dial.setPointerCapture(e.pointerId);
      dialDrag = true; dialLast = e.clientX;
      /* 开始拖动：恢复显示读数（上次松手后可能被还原按钮互斥隐藏） */
      label.style.display = 'block';
      resetBtn.style.display = 'none';
    };
    const onDialMove = e => {
      if (!dialDrag || !st.current.cropper) return;
      const dx = e.clientX - dialLast;
      dialLast = e.clientX;
      /* 拖动 2px ≈ 1°，限制在 ±180°（刻度不能溢出 180） */
      const next = Math.max(-180, Math.min(180, currentRot() + dx / 2));
      st.current.cropper.rotateTo(next);
      updateAngle();
    };
    const onDialUp = () => { dialDrag = false; updateMutual(); };
    dial.addEventListener('pointerdown', onDialDown);
    dial.addEventListener('pointermove', onDialMove);
    dial.addEventListener('pointerup', onDialUp);

    /* 还原：pointerdown + stopPropagation（wallDial 的 pointerdown preventDefault 会吞掉 click） */
    const onReset = e => {
      e.stopPropagation();
      e.preventDefault();
      if (!st.current.cropper) return;
      /* 还原：reset 归位（位置居中/旋转归零/原始缩放）→ 再应用缩小留边+框对齐 */
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

    /* 框内拖动 = 移图（捕获阶段拦截——裁剪框=全图时无处可拖，框内也必须能移图） */
    let innerDrag = false, innerX = 0, innerY = 0;
    const HANDLE = 16;   /* Cropper 手柄区域（px）——不拦截，让 resize 生效 */
    const onViewDown = e => {
      if (!st.current.cropper) return;
      /* touch：完全放行给 Cropper 原生（单指=移图、双指=缩放）——拦截会破坏双指手势 */
      if (e.pointerType === 'touch') return;
      const box = st.current.cropper.getCropBoxData();
      const r = view.getBoundingClientRect();
      const x = e.clientX - r.left, y = e.clientY - r.top;
      const inBox = x >= box.left && x <= box.left + box.width && y >= box.top && y <= box.top + box.height;
      const nearEdge = x < box.left + HANDLE || x > box.left + box.width - HANDLE ||
                       y < box.top + HANDLE || y > box.top + box.height - HANDLE;
      if (inBox && !nearEdge) {   /* 框内非手柄区——移图 */
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

  /* 把 capture 注册给 App（截屏按钮在 FloppyRow 中）——直接传函数，
   * 不包箭头函数（否则 App 端 setState(fn) 会把 fn 当 updater 意外调用） */
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
