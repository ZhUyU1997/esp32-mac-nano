import { useEffect, useRef } from 'preact/hooks';
import { sendMouseMove, sendMouseBtn } from './store.js';

const TAP_MOVE_PX = 25;   /* finger jitter tolerance for tap detection */
const TAP_MAX_MS = 400;   /* touch must be quick to count as a tap */
const DRAG_HOLD_MS = 250;

/* touchpad: slide=move, tap=left click, hold+slide=drag
 * 手势区域用原生 addEventListener（小写事件名）——绕开 Preact 对 touch 事件
 * 命名大小写的处理差异（桌面环境 'ontouchstart' in el = false 时事件名会被
 * 保留原大小写导致 handler 查不到），与 Cropper 同款命令式共存策略。 */
export function Touchpad() {
  const padRef = useRef(null);
  const hintRef = useRef(null);
  const st = useRef({ px: 0, py: 0, sx: 0, sy: 0, moved: false, dragging: false, longPressTimer: null, t0: 0 });

  useEffect(() => {
    const pad = padRef.current;
    const onStart = e => {
      e.preventDefault();
      if (hintRef.current) hintRef.current.style.display = 'none';
      if (e.touches.length >= 2) return;  /* ignore extra fingers (single-button Mac) */
      const t = e.changedTouches[0];
      st.current.sx = st.current.px = t.clientX;
      st.current.sy = st.current.py = t.clientY;
      st.current.t0 = Date.now();
      st.current.moved = false; st.current.dragging = false;
      st.current.longPressTimer = setTimeout(() => {
        st.current.longPressTimer = null;
        if (!st.current.moved) { st.current.dragging = true; sendMouseBtn(0, 1); }  /* hold -> begin drag */
      }, DRAG_HOLD_MS);
    };
    const onMove = e => {
      e.preventDefault();
      const t = e.changedTouches[0];
      const dx = t.clientX - st.current.px, dy = t.clientY - st.current.py;
      st.current.px = t.clientX; st.current.py = t.clientY;
      if (Math.abs(t.clientX - st.current.sx) + Math.abs(t.clientY - st.current.sy) > TAP_MOVE_PX) st.current.moved = true;
      if (st.current.moved && st.current.longPressTimer) { clearTimeout(st.current.longPressTimer); st.current.longPressTimer = null; }
      sendMouseMove(dx, dy);
    };
    const onEnd = e => {
      e.preventDefault();
      if (st.current.longPressTimer) { clearTimeout(st.current.longPressTimer); st.current.longPressTimer = null; }
      if (st.current.dragging) { st.current.dragging = false; sendMouseBtn(0, 0); }
      else if (!st.current.moved && Date.now() - st.current.t0 < TAP_MAX_MS) {
        sendMouseBtn(0, 1);
        setTimeout(() => sendMouseBtn(0, 0), 40);  /* real-click rhythm */
      }
    };
    pad.addEventListener('touchstart', onStart, { passive: false });
    pad.addEventListener('touchmove', onMove, { passive: false });
    pad.addEventListener('touchend', onEnd);
    return () => {
      pad.removeEventListener('touchstart', onStart);
      pad.removeEventListener('touchmove', onMove);
      pad.removeEventListener('touchend', onEnd);
    };
  }, []);

  return (
    <div id="pad" ref={padRef}>
      <div className="hint" ref={hintRef}>触控板<br />滑动 = 移动<br />轻点 = 左键<br />长按后滑动 = 拖动</div>
    </div>
  );
}
