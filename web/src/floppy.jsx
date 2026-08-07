import { useRef } from 'preact/hooks';
import { MFSVolume } from './mfs.js';
import { MacPaintImage } from './macpaint.js';
import { isConnected, upBusy, upTxt, shotBusy, showToast } from './store.js';

const UP_ORIG = '上传图片到软盘';
let imgSeq = 0;   /* module-level: each upload = fresh disk (numbering resets), survives re-render */

/* Upload images to floppy: each upload = fresh disk (packs currently selected images — no accumulation, no loading of old disks).
 * Upload is rejected while a floppy is inserted (eject first). */
export function FloppyRow({ onShot }) {
  const fileRef = useRef(null);

  async function floppyInserted() {
    try {
      const ac = new AbortController();
      const timer = setTimeout(() => ac.abort(), 5000);
      const resp = await fetch('/api/status', { signal: ac.signal });
      clearTimeout(timer);
      if (!resp.ok) return true;   /* conservative: if unknown, treat as inserted */
      const j = await resp.json();
      return !!j.floppy;
    } catch (e) {
      return true;                 /* network issue → conservative reject */
    }
  }

  async function onUpClick() {
    if (upBusy.value) return;
    if (!isConnected()) { showToast('WiFi 已断开，正在自动重连，请稍候再上传', true); return; }
    upBusy.value = true; upTxt.value = '检查中…';
    const inserted = await floppyInserted();
    if (inserted) {
      upBusy.value = false;
      showToast('软盘已占用，请将桌面上的软盘图标拖到废纸篓或点击File > Eject，再上传', true);
      return;
    }
    upBusy.value = false;
    fileRef.current.click();
  }

  async function onFileChange(e) {
    const input = e.currentTarget;
    if (!input.files || !input.files.length) return;
    if (!isConnected()) { showToast('WiFi 已断开，正在自动重连，请稍候再上传', true); input.value = ''; return; }
    if (await floppyInserted()) {
      showToast('软盘已占用，请将桌面上的软盘图标拖到废纸篓或点击File > Eject，再上传', true);
      input.value = '';
      return;
    }
    /* Max 7 images (per selection) */
    if (input.files.length > 7) {
      showToast('一个软盘最多 7 张图片，请重新选择后再上传', true);
      input.value = '';
      return;
    }
    /* Each upload = fresh disk: create new volume, reset numbering */
    imgSeq = 0;
    const vol = new MFSVolume({ create: true, sizeKB: 400, volumeName: 'IMAGES' });
    const total = input.files.length;
    upBusy.value = true;
    let ok = 0, fail = 0, capFull = false;
    try {
      /* Multiple images: convert to PNTG one by one into the new volume, then upload once */
      for (let i = 0; i < total; i++) {
        const file = input.files[i];
        upTxt.value = '处理中 ' + (i + 1) + '/' + total + '…';
        try {
          const bmp = await createImageBitmap(file);
          /* Uniform scale: width fills 576, height proportional */
          const sc = 576 / bmp.width;
          const w = 576;
          const h = Math.round(bmp.height * sc);
          const c = document.createElement('canvas');
          c.width = 576; c.height = 720;              /* PNTG canvas */
          const ctx = c.getContext('2d');
          ctx.fillStyle = '#ffffff'; ctx.fillRect(0, 0, 576, 720);
          ctx.drawImage(bmp, 0, 0, w, h);
          const imgData = ctx.getImageData(0, 0, 576, 720);
          const mp = new MacPaintImage({ width: 576, height: 720, data: imgData.data });
          const pntg = mp.toArrayBuffer();
          /* Capacity estimate: data blocks + 1 directory block — stop if insufficient (skip clearly oversized) */
          const freeB = (vol.volumeInfo && vol.volumeInfo.freeAllocBlocks) || 0;
          const need = Math.ceil(pntg.byteLength / 512) + 1;
          if (need > freeB) {
            capFull = true; fail++;
            break;   /* skip remaining images too */
          }
          const fileName = 'IMG' + String(++imgSeq).padStart(3, '0');
          vol.writeFile(fileName, pntg, null, { type: 'PNTG', creator: 'MPNT' });
          ok++;
        } catch (err) {
          /* Distinguish: insufficient capacity vs. image problem */
          if (err && /free block|capacity|space|Not enough/i.test(err.message || '')) capFull = true;
          fail++;
        }
      }
    } catch (err) {
      upBusy.value = false;
      showToast('图片处理失败，请换用 JPG/PNG 图片', true);
      input.value = '';
      return;
    }
    if (ok === 0) {
      upBusy.value = false;
      showToast('所选图片都无法处理，请换用 JPG/PNG 图片', true);
      input.value = '';
      return;
    }
    upTxt.value = '上传中…';
    try {
      const ac = new AbortController();
      const timer = setTimeout(() => ac.abort(), 10000);   /* 10s upload timeout */
      const resp = await fetch('/api/floppy', { method: 'POST', body: vol.imageBuffer, signal: ac.signal });
      clearTimeout(timer);
      if (!resp.ok) {
        /* backend error → localized message + hint */
        let why = '';
        try { why = (await resp.text()).trim(); } catch (err2) {}
        const map = {
          'image too large': '图片太大，请选择较小的图片',
          'no memory': '设备内存不足，请稍后重试或减少图片',
          'recv failed': '接收中断，请重试',
          'sd write failed': 'SD 写入失败，请检查 SD 卡后重试',
        };
        upBusy.value = false;
        showToast('上传失败：' + (map[why] || '服务器错误，请重试'), true);
        input.value = '';
        return;
      }
      upBusy.value = false;
      const failMsg = fail ? (capFull ? '（软盘容量不足，请少选几张或分两次上传，' + fail + ' 张未添加）' : '（' + fail + ' 张格式不支持已跳过，请用 JPG/PNG 图片）') : '';
      showToast('已上传 ' + ok + ' 张到软盘！双击桌面软盘图标查看' + failMsg);
    } catch (err) {
      upBusy.value = false;
      if (err && err.name === 'AbortError') showToast('上传超时，请重试或检查连接', true);
      else showToast('连接失败，请检查 WiFi 连接后重试', true);
    }
    input.value = '';                               /* allow re-selecting the same file */
  }

  return (
    <div className="floppyrow">
      <input type="file" id="imgFile" ref={fileRef} accept="image/*" multiple style={{ display: 'none' }} onChange={onFileChange} />
      <div className={'upbtn' + (upBusy.value ? ' busy' : '')} id="bUp" role="button" tabIndex="0" onClick={onUpClick}>
        <span className="upmain">{upBusy.value ? (upTxt.value || UP_ORIG) : UP_ORIG}</span>
      </div>
      <button className="shotbtn" id="bShot" title="截屏" disabled={shotBusy.value} onClick={onShot}></button>
    </div>
  );
}
