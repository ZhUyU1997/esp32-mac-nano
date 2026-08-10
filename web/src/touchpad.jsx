import { useEffect, useRef } from 'preact/hooks';
import { sendMouseMove, sendMouseBtn, sendClick } from './store.js';

const TAP_MOVE_PX = 30;   /* finger jitter tolerance for tap detection */
const DRAG_HOLD_MS = 250;  /* hold to start dragging (also the tap window —
                            * matches Guacamole's clickTimingThreshold) */

/* touchpad: slide=move, tap=left click, hold+slide=drag
 * Gesture area uses native addEventListener (lowercase event names) to bypass Preact's
 * handling of touch event name casing (on desktop where 'ontouchstart' in el = false,
 * the original casing is kept so handlers can't be found) — same imperative coexistence
 * strategy as Cropper. */
export function Touchpad() {
  const padRef = useRef(null);
  const hintRef = useRef(null);
  const st = useRef({ px: 0, py: 0, sx: 0, sy: 0, moved: false, dragging: false, longPressTimer: null });

  useEffect(() => {
    const pad = padRef.current;
    const onStart = e => {
      e.preventDefault();
      if (hintRef.current) hintRef.current.style.display = 'none';
      if (e.touches.length >= 2) return;  /* ignore extra fingers (single-button Mac) */
      const t = e.changedTouches[0];
      st.current.sx = st.current.px = t.clientX;
      st.current.sy = st.current.py = t.clientY;
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
      /* Only send mouse moves once the gesture is confirmed as a swipe/
       * drag: tap jitter (contact drift under the threshold) would otherwise
       * move the Mac cursor and interfere with the click that follows. */
      const overThreshold = Math.abs(t.clientX - st.current.sx) + Math.abs(t.clientY - st.current.sy) > TAP_MOVE_PX;
      if (st.current.moved || overThreshold) {
        st.current.moved = true;
        sendMouseMove(dx, dy);
      }
      if (st.current.moved && st.current.longPressTimer) { clearTimeout(st.current.longPressTimer); st.current.longPressTimer = null; }
    };
    const onEnd = e => {
      e.preventDefault();
      if (st.current.longPressTimer) { clearTimeout(st.current.longPressTimer); st.current.longPressTimer = null; }
      if (st.current.dragging) { st.current.dragging = false; sendMouseBtn(0, 0); }
      else if (!st.current.moved) {
        /* click semantics: the device guarantees the press duration —
         * no local down/up timing to tune or fight WS jitter with */
        sendClick();
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
